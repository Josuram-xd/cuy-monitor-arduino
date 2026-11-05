"""What the firmware prints, as readings: one JSON line {"grams": 812.4, "stable": true} each."""

import json
import logging
import math
from dataclasses import dataclass

logger = logging.getLogger(__name__)


@dataclass(frozen=True)
class Reading:
    grams: float
    stable: bool


def parse_line(line: str) -> Reading | None:
    """A reading, or None for anything that is not one. Never raises: a bad line is skipped, not fatal.

    Empty lines and diagnostics (starting with '#') are expected and skipped quietly; anything else that is
    not a valid reading is logged, because it means a firmware or cable problem.
    """
    text = line.strip()
    if not text or text.startswith("#"):
        return None
    try:
        data = json.loads(text)
    except json.JSONDecodeError:
        logger.warning("Skipping a line that is not JSON: %.60s", text)
        return None
    if not isinstance(data, dict):
        logger.warning("Skipping JSON that is not an object: %.60s", text)
        return None
    grams, stable = data.get("grams"), data.get("stable")
    # bool is an int in Python: "grams": true must not pass as 1 g
    if isinstance(grams, bool) or not isinstance(grams, int | float) or not isinstance(stable, bool):
        logger.warning("Skipping a reading without numeric grams and boolean stable: %.60s", text)
        return None
    if not math.isfinite(grams) or grams < 0:
        logger.warning("Skipping an impossible weight: %s g", grams)
        return None
    return Reading(round(float(grams), 1), stable)


class SendThrottle:
    """Lets through only stable readings, at most one per interval (AGENTS.md, bridge rule 2)."""

    def __init__(self, interval_seconds: float):
        self._interval = interval_seconds
        self._last_sent: float | None = None

    def should_send(self, reading: Reading, now: float) -> bool:
        if not reading.stable:
            return False
        if self._last_sent is not None and now - self._last_sent < self._interval:
            return False
        self._last_sent = now
        return True
