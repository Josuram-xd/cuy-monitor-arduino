"""Serial bridge: Arduino scale (USB) -> backend ingestion (HTTPS).

    python bridge.py

Reads the JSON lines of firmware/weight_sensor, keeps the stable readings (at most one every
SEND_INTERVAL_SECONDS) and posts them as WEIGHT events. Configuration in serial_bridge/.env
(see config.example.env). Stop it with Ctrl+C.
"""

import logging
import signal
import sys
import time
from collections.abc import Callable, Iterable

from config import ConfigError, Settings, load_settings
from delivery import BackendClient, Dispatcher
from events import weight_event
from readings import SendThrottle, parse_line
from serial_source import SerialLines

logger = logging.getLogger("serial_bridge")


class Bridge:
    """Wires the pieces: line -> reading -> throttle -> event -> queue -> backend."""

    def __init__(self, settings: Settings, dispatcher: Dispatcher, clock: Callable[[], float]):
        self._settings = settings
        self._dispatcher = dispatcher
        self._throttle = SendThrottle(settings.send_interval_seconds)
        self._clock = clock

    def handle(self, line: str | None) -> None:
        if line is not None:
            reading = parse_line(line)
            if reading is not None and self._throttle.should_send(reading, self._clock()):
                self._dispatcher.enqueue(weight_event(reading, self._settings.cage_id))
        # also on a quiet second: events kept while the backend was down go out as soon as it is back
        self._dispatcher.flush()

    def run(self, lines: Iterable[str | None]) -> None:
        for line in lines:
            self.handle(line)


def _stop_on_sigterm(signum, frame) -> None:
    # systemd, NSSM and Docker stop a service with SIGTERM: treat it like Ctrl+C so it closes cleanly
    raise KeyboardInterrupt


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    signal.signal(signal.SIGTERM, _stop_on_sigterm)
    try:
        settings = load_settings()
    except ConfigError as error:
        logger.error("Configuration: %s", error)
        return 2
    client = BackendClient.create(settings.backend_url, settings.api_key)
    dispatcher = Dispatcher(client, time.monotonic)
    logger.info(
        "Bridge for %s: %s -> %s every %s s",
        settings.cage_id,
        settings.serial_port,
        settings.backend_url,
        settings.send_interval_seconds,
    )
    try:
        Bridge(settings, dispatcher, time.monotonic).run(SerialLines(settings.serial_port).lines())
    except KeyboardInterrupt:
        logger.info("Stopped (%s events not sent)", dispatcher.pending)
    finally:
        client.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
