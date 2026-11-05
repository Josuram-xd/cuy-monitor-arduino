"""The common event envelope of the backend (cuy-monitor-backend/docs/contracts/events.md), type WEIGHT."""

from datetime import UTC, datetime
from uuid import uuid4

from readings import Reading

SOURCE = "arduino"
SCHEMA_VERSION = 1


def weight_event(reading: Reading, cage_id: str, measured_at: datetime | None = None) -> dict:
    """One envelope per reading. Its eventId is made here, once: a retry sends the same one, so the
    backend can ignore duplicates."""
    moment = (measured_at or datetime.now(UTC)).astimezone(UTC)
    return {
        "eventId": str(uuid4()),
        "type": "WEIGHT",
        "cageId": cage_id,
        "timestamp": moment.isoformat(timespec="seconds").replace("+00:00", "Z"),
        "source": SOURCE,
        "schemaVersion": SCHEMA_VERSION,
        "payload": {"grams": reading.grams, "stable": reading.stable},
    }
