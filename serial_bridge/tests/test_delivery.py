import json

import httpx
import pytest

from delivery import INGESTION_PATH, BackendClient, Dispatcher, Outcome
from events import weight_event
from readings import Reading


class FakeClock:
    def __init__(self):
        self.now = 0.0

    def __call__(self):
        return self.now


class FakeBackend:
    """Answers with the given statuses in order (or raises for 'down') and records what it received."""

    def __init__(self, *answers):
        self.answers = list(answers)
        self.received: list[httpx.Request] = []

    def __call__(self, request: httpx.Request) -> httpx.Response:
        self.received.append(request)
        answer = self.answers.pop(0) if self.answers else 202
        if answer == "down":
            raise httpx.ConnectError("connection refused", request=request)
        return httpx.Response(answer, json={})


def client_for(backend: FakeBackend) -> BackendClient:
    http = httpx.Client(base_url="https://backend.test", transport=httpx.MockTransport(backend))
    return BackendClient(http, "secret-key")


def event(grams=800.0):
    return weight_event(Reading(grams, True), "cage-1")


def test_an_event_is_posted_to_the_ingestion_endpoint_with_the_api_key():
    backend = FakeBackend(202)

    assert client_for(backend).send(event()) is Outcome.DELIVERED

    request = backend.received[0]
    assert request.method == "POST"
    assert request.url.path == INGESTION_PATH == "/api/v1/ingestion/events"
    assert request.headers["X-API-Key"] == "secret-key"


@pytest.mark.parametrize(
    ("answer", "outcome"),
    [
        (400, Outcome.DROP),
        (401, Outcome.DROP),
        (500, Outcome.RETRY),
        (503, Outcome.RETRY),
        ("down", Outcome.RETRY),
    ],
)
def test_4xx_is_dropped_and_5xx_or_network_errors_are_retried(answer, outcome):
    assert client_for(FakeBackend(answer)).send(event()) is outcome


def test_a_backend_that_is_down_is_retried_with_backoff_and_the_same_event_id():
    backend = FakeBackend("down", 503, 202)
    clock = FakeClock()
    dispatcher = Dispatcher(client_for(backend), clock)
    sent = event()
    dispatcher.enqueue(sent)

    assert dispatcher.flush() == 0  # down: wait 1 s
    clock.now = 0.5
    assert dispatcher.flush() == 0 and len(backend.received) == 1  # still waiting, not hammering
    clock.now = 1.0
    assert dispatcher.flush() == 0  # 503: wait 2 s more
    clock.now = 2.9
    dispatcher.flush()
    assert len(backend.received) == 2
    clock.now = 3.0
    assert dispatcher.flush() == 1
    assert dispatcher.pending == 0
    # every attempt carried the same eventId, so the backend can ignore duplicates
    assert {json.loads(r.content)["eventId"] for r in backend.received} == {sent["eventId"]}


def test_the_backoff_doubles_up_to_a_minute():
    backend = FakeBackend(*["down"] * 10)
    clock = FakeClock()
    dispatcher = Dispatcher(client_for(backend), clock)
    dispatcher.enqueue(event())
    waits = []
    for _ in range(9):
        before = clock.now
        dispatcher.flush()
        clock.now = dispatcher._next_attempt
        waits.append(clock.now - before)

    assert waits == [1, 2, 4, 8, 16, 32, 60, 60, 60]


def test_a_refused_event_is_dropped_and_the_next_one_goes_out():
    backend = FakeBackend(400, 202)
    dispatcher = Dispatcher(client_for(backend), FakeClock())
    dispatcher.enqueue(event(1.0))
    dispatcher.enqueue(event(2.0))

    assert dispatcher.flush() == 1
    assert dispatcher.pending == 0
    assert json.loads(backend.received[1].content)["payload"]["grams"] == 2.0


def test_the_queue_keeps_the_newest_events_when_it_is_full():
    dispatcher = Dispatcher(client_for(FakeBackend()), FakeClock(), max_queue=3)
    events = [event(float(g)) for g in range(5)]
    for e in events:
        dispatcher.enqueue(e)

    assert dispatcher.pending == 3
    assert [e["payload"]["grams"] for e in dispatcher._queue] == [2.0, 3.0, 4.0]


def test_the_whole_backlog_goes_out_in_order_once_the_backend_is_back():
    backend = FakeBackend("down")
    clock = FakeClock()
    dispatcher = Dispatcher(client_for(backend), clock)
    for g in (1.0, 2.0, 3.0):
        dispatcher.enqueue(event(g))
    dispatcher.flush()

    clock.now = 1.0
    assert dispatcher.flush() == 3
    assert [json.loads(r.content)["payload"]["grams"] for r in backend.received[1:]] == [1.0, 2.0, 3.0]
