"""Lines from the Arduino's USB serial port, reopening it when it is unplugged (AGENTS.md, bridge rule 4)."""

import logging
import time
from collections.abc import Callable, Iterator
from typing import Any

import serial

from config import BAUD_RATE

logger = logging.getLogger(__name__)

REOPEN_SECONDS = 5.0
READ_TIMEOUT_SECONDS = 1.0


class SerialLines:
    """Yields one decoded line per reading, or None when a second passed without one, so the caller can
    still retry pending events while the scale is quiet. It never gives up: if the port is missing or the
    USB is pulled out, it waits REOPEN_SECONDS and opens it again."""

    def __init__(
        self,
        port: str,
        opener: Callable[..., Any] = serial.Serial,
        sleep: Callable[[float], None] = time.sleep,
    ):
        self._port = port
        self._opener = opener
        self._sleep = sleep

    def _open(self) -> Any:
        while True:
            try:
                connection = self._opener(self._port, BAUD_RATE, timeout=READ_TIMEOUT_SECONDS)
                logger.info("Serial port %s open", self._port)
                return connection
            except (serial.SerialException, OSError) as error:
                logger.warning("Can't open %s (%s): trying again in %s s", self._port, error, REOPEN_SECONDS)
                self._sleep(REOPEN_SECONDS)

    def lines(self) -> Iterator[str | None]:
        while True:
            connection = self._open()
            try:
                while True:
                    raw = connection.readline()
                    yield raw.decode("utf-8", errors="replace") if raw else None
            except (serial.SerialException, OSError) as error:
                logger.warning("Lost %s (%s): reopening it", self._port, error)
            finally:
                try:
                    connection.close()
                except serial.SerialException, OSError:
                    pass
            self._sleep(REOPEN_SECONDS)
