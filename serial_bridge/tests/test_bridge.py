import json

import httpx

from bridge import Bridge
from config import Settings
from delivery import BackendClient, Dispatcher

SETTINGS = Settings("COM3", "https://cuymonitor.duckdns.org", "key", "cage-1", 10)


class Clock:
    def __init__(self):
        self.now = 0.0

    def __call__(self):
        return self.now


def test_from_serial_lines_to_weight_events_end_to_end():
    received = []

    def backend(request):
        received.append(json.loads(request.content))
        return httpx.Response(202, json={"status": "ACCEPTED"})

    clock = Clock()
    http = httpx.Client(base_url=SETTINGS.backend_url, transport=httpx.MockTransport(backend))
    bridge = Bridge(SETTINGS, Dispatcher(BackendClient(http, SETTINGS.api_key), clock), clock)
    # what the firmware prints over 30 s: a diagnostic, a moving reading, then stable ones every 2 s
    script = [
        (0, "# weight sensor ready\n"),
        (2, '{"grams": 1250.0, "stable": false}\n'),
        (4, '{"grams": 1000.0, "stable": true}\n'),
        (6, '{"grams": 1000.1, "stable": true}\n'),
        (8, None),
        (10, "garbage\n"),
        (14, '{"grams": 1000.3, "stable": true}\n'),
        (16, '{"grams": 999.9, "stable": true}\n'),
        (26, '{"grams": 1002.0, "stable": true}\n'),
    ]
    for at, line in script:
        clock.now = at
        bridge.handle(line)

    assert [event["payload"]["grams"] for event in received] == [1000.0, 1000.3, 1002.0]
    assert all(event["type"] == "WEIGHT" and event["cageId"] == "cage-1" for event in received)


def test_readings_taken_while_the_backend_is_down_go_out_when_it_is_back():
    answers = ["down", "down"]
    received = []

    def backend(request):
        if answers:
            answers.pop()
            raise httpx.ConnectError("no route", request=request)
        received.append(json.loads(request.content)["eventId"])
        return httpx.Response(202, json={})

    clock = Clock()
    http = httpx.Client(base_url=SETTINGS.backend_url, transport=httpx.MockTransport(backend))
    dispatcher = Dispatcher(BackendClient(http, SETTINGS.api_key), clock)
    bridge = Bridge(SETTINGS, dispatcher, clock)

    bridge.handle('{"grams": 500.0, "stable": true}\n')
    clock.now = 10
    bridge.handle('{"grams": 510.0, "stable": true}\n')
    assert dispatcher.pending == 2

    for second in range(11, 20):  # quiet seconds: the queue is retried on its own
        clock.now = second
        bridge.handle(None)

    assert dispatcher.pending == 0 and len(received) == 2
