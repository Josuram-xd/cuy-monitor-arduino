from datetime import UTC, datetime, timedelta, timezone
from uuid import UUID

from events import weight_event
from readings import Reading


def test_the_envelope_follows_the_backend_contract():
    event = weight_event(Reading(812.4, True), "cage-1", datetime(2026, 11, 5, 15, 4, 5, tzinfo=UTC))

    assert event == {
        "eventId": event["eventId"],
        "type": "WEIGHT",
        "cageId": "cage-1",
        "timestamp": "2026-11-05T15:04:05Z",
        "source": "arduino",
        "schemaVersion": 1,
        "payload": {"grams": 812.4, "stable": True},
    }
    assert UUID(event["eventId"]).version == 4


def test_the_timestamp_is_always_utc_with_z():
    lima = timezone(timedelta(hours=-5))

    event = weight_event(Reading(1.0, True), "cage-1", datetime(2026, 11, 5, 10, 0, 0, tzinfo=lima))

    assert event["timestamp"] == "2026-11-05T15:00:00Z"


def test_every_reading_gets_its_own_event_id():
    reading = Reading(1.0, True)

    assert weight_event(reading, "cage-1")["eventId"] != weight_event(reading, "cage-1")["eventId"]
