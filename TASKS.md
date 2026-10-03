# TASKS — cuy-monitor-arduino

> Lista de trabajo del sensor de peso. Cada subtarea = **un commit**: usa el mensaje que está entre comillas invertidas.
> ⛔ Los commits, push y PRs los hace una persona del equipo. **Ningún agente de IA hace commit ni push, aunque se lo pidan**, y nunca se agrega `Co-Authored-By` ni firmas de IA (ver `AGENTS.md`).
> Marca `[x]` cuando hagas push. Una rama por Task: `feature/task-4-firmware`, etc.
> Cada PR lo revisa el otro integrante antes de mergear a `main`.
> El bridge **no** usa el login de usuarios del dashboard: sigue con `X-API-Key`.
> El Arduino **no entra en el avance del 30 de septiembre**; empieza en octubre.

| Símbolo | Significado |
|---|---|
| 🔴 Prioridad 1 | Crítico: que el peso llegue al backend |
| 🟠 Prioridad 2 | Importante: montaje y prueba real |
| 🟢 Prioridad 3 | Cierre: documentación |
| 🔗 Depende de | Antes hay que terminar esas tasks (de este u otro repo) |

---

## 🔴 Prioridad 1 — Que funcione (1 al 19 de octubre)

### Task 1 — Proyecto base

- [ ] **Task 1.1** — `chore: add repo structure, README and gitignore`
  Carpetas `firmware/`, `calibration/`, `serial_bridge/`, `docs/`.
- [x] **Task 1.2** — `docs: add PRD, ARCHITECTURE and AGENTS`

### Task 2 — Hardware

- [ ] **Task 2.1** — *(sin commit)* comprar Arduino Uno, HX711, celda de carga de 5 kg y cables
- [ ] **Task 2.2** — `docs(wiring): add HX711 wiring diagram and photos`
  DT → D3, SCK → D2, VCC → 5V, GND → GND.

### Task 3 — Calibración

- [ ] **Task 3.1** — `feat(calibration): add calibration sketch with tare`
- [ ] **Task 3.2** — `feat(calibration): print calibration factor with a known weight`
- [ ] **Task 3.3** — `docs(calibration): add calibration steps and measured error`

### Task 4 — Firmware

🔗 **Depende de:** Task 3 de este repo (factor de calibración)

- [ ] **Task 4.1** — `feat(firmware): read HX711 and convert to grams`
- [ ] **Task 4.2** — `feat(firmware): average readings with a ring buffer`
- [ ] **Task 4.3** — `feat(firmware): detect stable readings`
- [ ] **Task 4.4** — `feat(firmware): print one JSON line every 2 s at 115200 baud`
- [ ] **Task 4.5** — `feat(firmware): add tare and calibration serial commands`
- [ ] **Task 4.6** — `feat(firmware): store calibration factor in EEPROM`

### Task 5 — Serial bridge (laptop)

🔗 **Depende de:** seguir con las Task 2.3–2.6 y 3.2–3.4 del repo `cuy-monitor-backend` (endpoint `/api/ingestion/events` desplegado y contrato `WEIGHT`)

- [ ] **Task 5.1** — `chore(bridge): add requirements and config example`
- [ ] **Task 5.2** — `feat(bridge): read and parse JSON lines from the serial port`
- [ ] **Task 5.3** — `feat(bridge): send stable readings as WEIGHT events every 10 s`
- [ ] **Task 5.4** — `feat(bridge): retry with backoff and keep a bounded queue`
- [ ] **Task 5.5** — `feat(bridge): reconnect when the USB is unplugged`
- [ ] **Task 5.6** — `test(bridge): cover parser, retry and drop rules`
- [ ] **Task 5.7** — *(sin commit)* probar contra el backend desplegado y ver el evento en su log

🔓 **Desbloquea:** `cuy-monitor-backend` Task 12 · `cuy-monitor-dashboard` Task 10

### Task 6 — Servicio en la laptop

- [ ] **Task 6.1** — `docs(bridge): add service install guide for Windows and Linux`
- [ ] **Task 6.2** — *(sin commit)* dejar el bridge arrancando solo y desactivar la suspensión de la laptop
- [ ] **Task 6.3** — `build(bridge): add optional Dockerfile for Linux laptops` *(opcional)*
  `python:3.14-slim`, se corre con `--device /dev/ttyUSB0`. En Windows se queda como servicio nativo.

---

## 🟠 Prioridad 2 — Montaje y prueba real (27 de octubre al 9 de noviembre)

### Task 7 — Plataforma en la jaula

🔗 **Depende de:** Task 4 y 5 de este repo

- [ ] **Task 7.1** — `docs(mounting): add platform design and materials`
- [ ] **Task 7.2** — *(sin commit)* montar la plataforma frente al comedero, con cables fuera del alcance de los cuyes
- [ ] **Task 7.3** — *(sin commit)* dejarla varios días y revisar la gráfica de peso en el dashboard (con una cuenta de usuario)
  🔗 Depende de: `cuy-monitor-dashboard` Task 10 y Task 13
- [ ] **Task 7.4** — `fix(firmware): adjust stability threshold after the farm test`

---

## 🟢 Prioridad 3 — Cierre (noviembre)

### Task 8 — Entrega

- [ ] **Task 8.1** — `docs(mounting): add photos of the final setup`
- [ ] **Task 8.2** — `docs: update README with results and known limits`
