"""Sends the events to the backend's ingestion endpoint, keeping the ones that could not be sent yet."""

import logging
from collections import deque
from collections.abc import Callable
from enum import Enum, auto

import httpx

from config import MAX_QUEUE

logger = logging.getLogger(__name__)

INGESTION_PATH = "/api/v1/ingestion/events"
FIRST_BACKOFF_SECONDS = 1.0
MAX_BACKOFF_SECONDS = 60.0


class Outcome(Enum):
    DELIVERED = auto()
    DROP = auto()  # the backend refused it (4xx): sending it again would get the same answer
    RETRY = auto()  # network error or 5xx: it may work later


class BackendClient:
    def __init__(self, http: httpx.Client, api_key: str):
        self._http = http
        self._api_key = api_key

    @classmethod
    def create(cls, backend_url: str, api_key: str) -> BackendClient:
        # certificates are always verified (httpx default): never turned off
        return cls(httpx.Client(base_url=backend_url, timeout=10.0), api_key)

    def send(self, event: dict) -> Outcome:
        try:
            response = self._http.post(INGESTION_PATH, json=event, headers={"X-API-Key": self._api_key})
        except httpx.HTTPError as error:
            logger.warning("Backend not reachable (%s): event %s kept", error, event["eventId"])
            return Outcome.RETRY
        if response.status_code == 202:
            logger.info("Weight %s g sent (%s)", event["payload"]["grams"], event["eventId"])
            return Outcome.DELIVERED
        if response.status_code >= 500:
            logger.warning("Backend answered %s: event %s kept", response.status_code, event["eventId"])
            return Outcome.RETRY
        logger.error(
            "Backend refused event %s with %s (%s): dropped",
            event["eventId"],
            response.status_code,
            response.text[:200],
        )
        return Outcome.DROP

    def close(self) -> None:
        self._http.close()


class Dispatcher:
    """A bounded queue in front of the backend.

    Events go out oldest first. When one can't be sent, everything waits with exponential backoff
    (1 s, 2 s, 4 s … up to 60 s), so a backend that is down is not hammered. When the queue is full, the
    oldest event is dropped: the newest readings are the most useful.
    """

    def __init__(
        self,
        client: BackendClient,
        clock: Callable[[], float],
        max_queue: int = MAX_QUEUE,
    ):
        self._client = client
        self._clock = clock
        self._queue: deque[dict] = deque()
        self._max_queue = max_queue
        self._backoff = 0.0
        self._next_attempt = 0.0

    @property
    def pending(self) -> int:
        return len(self._queue)

    def enqueue(self, event: dict) -> None:
        if len(self._queue) >= self._max_queue:
            dropped = self._queue.popleft()
            logger.warning(
                "Queue full (%s): dropping the oldest event %s", self._max_queue, dropped["eventId"]
            )
        self._queue.append(event)

    def flush(self) -> int:
        """Sends what it can now; returns how many events were delivered."""
        delivered = 0
        while self._queue and self._clock() >= self._next_attempt:
            outcome = self._client.send(self._queue[0])
            if outcome is Outcome.RETRY:
                self._backoff = min(MAX_BACKOFF_SECONDS, self._backoff * 2 or FIRST_BACKOFF_SECONDS)
                self._next_attempt = self._clock() + self._backoff
                break
            self._queue.popleft()
            self._backoff = 0.0
            if outcome is Outcome.DELIVERED:
                delivered += 1
        return delivered
