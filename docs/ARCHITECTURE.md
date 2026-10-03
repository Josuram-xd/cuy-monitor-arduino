# Architecture — cuy-monitor-arduino

> Arduino Uno (ATmega328P) · Arduino IDE 2.x · HX711 library (bogde) · Python 3.14 + pyserial + httpx on the farm laptop
> Last reviewed: 2026-10-03

## 1. Context

```
┌──────────────── cage ─────────────────┐     ┌──────── farm laptop ────────┐        ┌──── AWS ────┐
│ platform                              │     │                             │ HTTPS  │             │
│  └─ 5 kg load cell ─► HX711 ─► Arduino│ USB │ serial_bridge (Python)      │ ─────► │ Caddy (EC2) │
│                     (DT=3, SCK=2)     │ ──► │  reads JSON lines           │ X-API- │  └► backend │
└───────────────────────────────────────┘     │ POST /api/ingestion/events  │  Key   │      └► RDS │
                                              └─────────────────────────────┘        └─────────────┘
```

The Arduino Uno has no network, so the laptop acts as a bridge. It calls the backend's single ingestion endpoint over HTTPS with the common event envelope (`type: WEIGHT`).

- The bridge authenticates with `X-API-Key`. User login (JWT) is only for people using the dashboard; it doesn't apply here.
- The backend stores readings in Amazon RDS (schema owned by `cuy-monitor-db`). The bridge never talks to the database.

## 2. Repository layout

```
cuy-monitor-arduino/
├── firmware/weight_sensor/weight_sensor.ino   reads HX711, averages, stability, JSON over serial
├── calibration/calibrate/calibrate.ino        finds the calibration factor with a known weight
├── serial_bridge/
│   ├── bridge.py                              pyserial → HTTPS POST
│   ├── config.example.env                     SERIAL_PORT, BACKEND_URL, API_KEY, CAGE_ID
│   ├── requirements.txt
│   └── Dockerfile                             optional, Linux laptops only (python:3.14-slim)
└── docs/
    ├── wiring.md
    └── mounting.md
```

Arduino requires each `.ino` to live in a folder with the same name.

## 3. Hardware and wiring

| HX711 pin | Arduino Uno |
|---|---|
| VCC | 5V |
| GND | GND |
| DT (DOUT) | D3 |
| SCK | D2 |

Load cell → HX711: red E+, black E−, white A−, green A+ (check the cell's datasheet; colors vary). Channel A, gain 128.

## 4. Firmware (`weight_sensor.ino`)

### Loop

```
every ~200 ms: read HX711 (scale.get_units()) → push into ring buffer (N = 10)
every ~2 s:
    grams  = mean(buffer)
    stable = (max(buffer) - min(buffer)) <= STABLE_DELTA_G   // e.g. 5 g
    print one JSON line: {"grams": 812.4, "stable": true}
```

### Serial protocol

- Baud rate **115200**, 8N1, one JSON object per line terminated by `\n`.
- Output: `{"grams": <float, 1 decimal>, "stable": <bool>}`.
- Diagnostic lines start with `#` (e.g. `# tare done`) and must be ignored by the bridge.
- Input commands (single char + `\n`):

| Command | Action |
|---|---|
| `t` | Tare (set current load as zero) |
| `c <factor>` | Set calibration factor at runtime (not persisted unless saved to EEPROM) |

### Rules

- No `delay()` longer than the HX711 read interval; use `millis()` timing.
- Calibration factor stored in EEPROM so a power cut doesn't lose it.
- Negative values below −20 g are clamped and flagged `stable: false` (usually a tare problem).

## 5. Calibration (`calibrate.ino`)

1. Empty platform → tare.
2. Place a known weight (e.g. 500 g bag/water bottle weighed on a kitchen scale).
3. Adjust the factor until the reading matches; the sketch prints the final factor.
4. Save it in EEPROM (or paste it into `weight_sensor.ino` as the default).
5. Re-check with a second known weight; target error ≤ ±5 g.

## 6. Serial bridge (`serial_bridge/bridge.py`)

```
open SERIAL_PORT @115200 (retry every 5 s if unplugged)
for each line:
    skip empty lines and lines starting with '#'
    parse JSON; skip if invalid
    if stable and (now - last_sent) >= SEND_INTERVAL (10 s):
        body = envelope { eventId: uuid4, type: WEIGHT, cageId, timestamp: now UTC,
                          source: arduino, schemaVersion: 1, payload: { grams, stable } }
        POST {BACKEND_URL}/api/ingestion/events  header X-API-Key
        on network error / 5xx → append to bounded queue (max 1000, drop oldest), retry with backoff
        on 4xx → log and drop (bad data or wrong key; retrying won't help)
```

### Request contract (source of truth: `cuy-monitor-backend/docs/contracts/`)

```http
POST /api/ingestion/events
X-API-Key: <API_KEY>
Content-Type: application/json

{
  "eventId": "9b2c4e1a-6f0d-4c1e-8a4b-2f1d3e5a7c90",
  "type": "WEIGHT",
  "cageId": "cage-1",
  "timestamp": "2026-10-12T15:04:05Z",
  "source": "arduino",
  "schemaVersion": 1,
  "payload": { "grams": 812.4, "stable": true }
}
```

The `eventId` is generated once per reading and kept when retrying, so the backend can ignore duplicates. Responses: `202` ok · `400`/`401` drop · `5xx`/network error retry.

### Configuration (`serial_bridge/.env`, never committed)

| Variable | Example |
|---|---|
| `SERIAL_PORT` | `COM3` (Windows) / `/dev/ttyUSB0` or `/dev/ttyACM0` (Linux) |
| `BACKEND_URL` | `https://cuymonitor.duckdns.org` |
| `API_KEY` | same `API_KEY` as the backend `infra/.env` |
| `CAGE_ID` | `cage-1` |
| `SEND_INTERVAL_SECONDS` | `10` |

### Running as a service

- Linux: systemd unit with `Restart=always`, **or** the optional container: `docker run -d --restart unless-stopped --device /dev/ttyUSB0 --env-file serial_bridge/.env cuy-monitor-serial-bridge:local`.
- Windows: NSSM or Task Scheduler "at startup", with power-saving/sleep disabled. Docker is not used on Windows (USB serial passthrough is unreliable and Docker Desktop is too heavy for the Celeron).
- The bridge stays on the farm laptop: it can't run on AWS because it needs the USB port.

## 7. Mounting (summary of `docs/mounting.md`)

- Rigid base + top plate, load cell in between, sized so the plate doesn't touch the cage walls.
- Non-slip, washable surface; rounded edges.
- Electronics and cables outside the cage or in a closed box the animals can't chew.
- Place it where guinea pigs walk regularly (e.g. in front of the feeder) so it gets frequent stable readings.

## 8. Testing

| What | How |
|---|---|
| Firmware | Serial Monitor: stable readings, tare command, known weights |
| Bridge parsing | pytest with sample lines (valid JSON, `#` lines, garbage) |
| Bridge offline behavior | Point `BACKEND_URL` to an unreachable host, check the queue and resend |
| End to end | Reading appears in `GET /api/cages/cage-1/weight` (with a user JWT) and on the dashboard chart after logging in |

## 9. Decisions

| Decision | Why |
|---|---|
| USB serial + laptop bridge | Uno has no WiFi; laptop is already on site |
| Same ingestion endpoint as the ai-service | One contract for every producer (backend ADR-003) |
| Cage-level weight | No reliable way to know which guinea pig is on the plate yet |
| Send only stable readings, max 1 / 10 s | Less noise and bandwidth; the backend doesn't need raw samples |
| Future: ESP32 | Would send over WiFi directly and remove the bridge |
