import serial

from serial_source import REOPEN_SECONDS, SerialLines


class FakePort:
    """A serial port that gives some lines and then, optionally, is unplugged."""

    def __init__(self, lines, unplug_after=False):
        self._lines = list(lines)
        self._unplug_after = unplug_after
        self.closed = False

    def readline(self):
        if self._lines:
            return self._lines.pop(0)
        if self._unplug_after:
            raise serial.SerialException("device reports readiness to read but returned no data")
        return b""

    def close(self):
        self.closed = True


class Opener:
    """Fails while the cable is not plugged in, then returns the ports in order."""

    def __init__(self, failures, *ports):
        self.failures = failures
        self.ports = list(ports)
        self.calls = []

    def __call__(self, port, baud, timeout):
        self.calls.append((port, baud, timeout))
        if self.failures:
            self.failures -= 1
            raise serial.SerialException(f"could not open port {port}")
        return self.ports.pop(0)


def take(iterator, count):
    return [next(iterator) for _ in range(count)]


def test_it_waits_for_the_usb_and_opens_the_port_at_115200():
    sleeps = []
    opener = Opener(2, FakePort([b'{"grams": 1.0, "stable": true}\n']))

    lines = take(SerialLines("COM3", opener=opener, sleep=sleeps.append).lines(), 1)

    assert lines == ['{"grams": 1.0, "stable": true}\n']
    assert sleeps == [REOPEN_SECONDS, REOPEN_SECONDS] == [5.0, 5.0]
    assert opener.calls[-1] == ("COM3", 115200, 1.0)


def test_a_quiet_second_yields_none_so_pending_events_can_still_go_out():
    opener = Opener(0, FakePort([b"", b'{"grams": 2.0, "stable": true}\n']))

    assert take(SerialLines("COM3", opener=opener, sleep=lambda s: None).lines(), 2) == [
        None,
        '{"grams": 2.0, "stable": true}\n',
    ]


def test_an_unplugged_usb_is_reopened_without_stopping():
    sleeps = []
    first = FakePort([b"# weight sensor ready\n"], unplug_after=True)
    second = FakePort([b'{"grams": 3.0, "stable": true}\n'])
    opener = Opener(0, first, second)

    lines = take(SerialLines("/dev/ttyUSB0", opener=opener, sleep=sleeps.append).lines(), 2)

    assert lines == ["# weight sensor ready\n", '{"grams": 3.0, "stable": true}\n']
    assert first.closed
    assert sleeps == [REOPEN_SECONDS]


def test_bytes_that_are_not_utf8_do_not_stop_it():
    opener = Opener(0, FakePort([b"\xff\xfe noise\n"]))

    assert take(SerialLines("COM3", opener=opener, sleep=lambda s: None).lines(), 1)[0].endswith("noise\n")
