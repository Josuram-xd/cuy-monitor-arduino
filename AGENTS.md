# AGENTS.md — cuy-monitor-arduino

Instrucciones para cualquier agente de IA que trabaje en este repo. Léelas completas antes de tocar código.

## Qué es este repo

El sensor de peso del Monitor de Salud de Cuyes:
- `firmware/` y `calibration/`: sketches de **Arduino Uno** (C++) que leen una celda de carga con el HX711 y mandan el peso por USB serial.
- `serial_bridge/`: programa en **Python 3.14** que corre en la laptop del criadero, lee el puerto serial y manda las lecturas al backend por HTTPS.

Lee antes de trabajar:
- `docs/PRD.md` — qué hace y qué no.
- `docs/ARCHITECTURE.md` — cableado, protocolo serial, bridge, contrato.
- `cuy-monitor-backend/docs/contracts/` — formato del payload `WEIGHT` y del endpoint de ingesta. **Fuente de verdad.**

## Comandos

```bash
# Firmware (Arduino CLI; o abrir la carpeta en Arduino IDE 2)
arduino-cli compile --fqbn arduino:avr:uno firmware/weight_sensor
arduino-cli upload  --fqbn arduino:avr:uno -p COM3 firmware/weight_sensor

# Bridge
cd serial_bridge
pip install -r requirements.txt
python bridge.py
pytest
ruff check .
```

## Idioma y nombres

- Código, comentarios y commits en **inglés**.
- Python en `snake_case`; los campos JSON que se mandan al backend en `camelCase` (`cageId`, `measuredAt`).
- La documentación del equipo puede estar en español.

## Reglas del firmware

1. El Arduino Uno tiene **2 KB de RAM**: nada de `String` dinámicos en el loop, nada de arreglos grandes. Usa `char[]` y `snprintf`/`dtostrf`.
2. Nada de `delay()` largos: temporiza con `millis()`.
3. Protocolo serial fijo: **115200 baudios**, una línea JSON por lectura `{"grams": 812.4, "stable": true}`. Las líneas de diagnóstico empiezan con `#`. No cambies este formato sin actualizar también `bridge.py` y la documentación.
4. Pines: HX711 **DT → D3**, **SCK → D2**. No los cambies sin actualizar `docs/wiring.md`.
5. Cada `.ino` va dentro de una carpeta con su mismo nombre (lo exige Arduino).
6. El factor de calibración se guarda en EEPROM; no lo dejes solo en una variable.

## Reglas del bridge

1. El bridge solo habla con `POST /api/ingestion/events` del backend, mandando el sobre común con `type: "WEIGHT"`.
2. Solo manda lecturas con `stable: true`, máximo una cada 10 s.
3. Si falla la red o el backend responde 5xx: guarda en una cola **limitada** (máx. 1000) y reintenta con backoff. Si responde 4xx: registra el error y descarta (reintentar no sirve).
4. Si se desconecta el USB, reintenta abrir el puerto cada 5 s sin cerrarse.
5. Ignora líneas vacías, líneas con `#` y JSON inválido sin caerse.
6. Tiene que funcionar en **Windows y Linux** (la laptop puede tener cualquiera): nada de rutas o comandos específicos de un solo sistema.

## Seguridad

- La `API_KEY` va en `serial_bridge/.env`. Nunca en el código ni en commits. Solo se sube `config.example.env`.
- Siempre HTTPS hacia el backend; no desactives la verificación de certificados.

## Bienestar animal

Cualquier cambio en el montaje (`docs/mounting.md`) tiene que mantener: plataforma firme, bordes redondeados, cables y electrónica fuera del alcance de los cuyes. Si una propuesta pone en riesgo a los animales, no la hagas y avísale al usuario.

## Tests

- Antes de decir que terminaste: `pytest` y `ruff check` del bridge sin errores; el sketch compila con `arduino-cli compile`.
- Parser del bridge probado con líneas válidas, líneas `#`, JSON roto y líneas vacías.

## Git

- Conventional Commits en inglés: `feat(firmware): add tare command`, `fix(bridge): reconnect on USB unplug`, `docs(wiring): add photos`.
- `main` solo por Pull Request. **Prohibido** `git push --force` a `main`.

## Lo que el agente NO debe hacer sin permiso explícito

- Cambiar el protocolo serial, los pines o el contrato con el backend.
- Subir un sketch a una placa conectada (`arduino-cli upload`): puede estar montada en la jaula.
- Agregar librerías de Arduino distintas a HX711 (bogde).
- Hacer push o abrir PRs.

## Dueño

Todo el repo: **el compañero**. Josuram revisa los PRs.

## Herramientas que puede usar el agente

- Leer y editar archivos del repo.
- `arduino-cli compile` (no `upload` sin permiso).
- `pip`, `pytest`, `ruff` y correr `bridge.py` contra un backend de prueba.
- `git status`, `git diff`, `git log`, ramas y commits locales.
