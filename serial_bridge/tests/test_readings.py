import pytest

from readings import Reading, SendThrottle, parse_line


@pytest.mark.parametrize(
    ("line", "expected"),
    [
        ('{"grams": 812.4, "stable": true}\n', Reading(812.4, True)),
        ('{"grams": 0.0, "stable": false}\r\n', Reading(0.0, False)),
        ('{"grams": 1000, "stable": true}', Reading(1000.0, True)),
        ('{"grams": 812.46, "stable": true}', Reading(812.5, True)),
    ],
)
def test_a_reading_of_the_firmware_is_parsed(line, expected):
    assert parse_line(line) == expected


@pytest.mark.parametrize(
    "line",
    [
        "",
        "\n",
        "   ",
        "# tare done",
        "# factor 420.0000 from EEPROM",
        '{"grams": 812.4, "stable": tr',  # cut in the middle: the USB was pulled
        "\x00\xff garbage",
        "[1, 2]",
        '{"grams": "812", "stable": true}',
        '{"grams": true, "stable": true}',
        '{"grams": 812.4}',
        '{"grams": 812.4, "stable": "yes"}',
        '{"grams": -5.0, "stable": true}',
        '{"grams": NaN, "stable": true}',
        '{"grams": Infinity, "stable": true}',
    ],
)
def test_anything_else_is_skipped_without_raising(line):
    assert parse_line(line) is None


def test_only_stable_readings_go_out_and_at_most_one_per_interval():
    throttle = SendThrottle(10)
    stable, moving = Reading(800.0, True), Reading(800.0, False)

    assert not throttle.should_send(moving, now=0)
    assert throttle.should_send(stable, now=1)
    assert not throttle.should_send(stable, now=5)
    assert not throttle.should_send(stable, now=10.9)
    assert throttle.should_send(stable, now=11)
    assert not throttle.should_send(moving, now=30)
    assert throttle.should_send(stable, now=30)
