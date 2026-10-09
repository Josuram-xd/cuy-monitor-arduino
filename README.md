# Cuy Monitor — sensor de peso

Firmware para leer una celda de carga con HX711 y Arduino Uno, más un bridge
que envía las lecturas estables al backend de Cuy Monitor.

## Estructura

```text
firmware/weight_sensor/   Sketch del sensor
calibration/calibrate/    Sketch para calibrar con un peso conocido
serial_bridge/           Bridge Python de USB serial a HTTPS
docs/                    Cableado, arquitectura y montaje
```

## Estado

La estructura y la documentación inicial están preparadas. Las tareas de
calibración, firmware y bridge se implementarán por separado; consulta
[`TASKS.md`](TASKS.md) para el estado.

## Documentación

- [`docs/PRD.md`](docs/PRD.md): requisitos del sensor y del bridge.
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md): protocolo serial e integración.

No conectes el hardware ni envíes lecturas a producción hasta completar la
calibración y configurar `serial_bridge/.env` localmente.
