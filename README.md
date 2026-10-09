# Cuy Monitor — sensor de peso

Firmware para leer una celda de carga con HX711 y Arduino Uno, más un bridge
que envía las lecturas estables al backend de Cuy Monitor.

## Estructura

```text
firmware/weight_sensor/   Sketch del sensor
calibration/calibrate/    Sketch para calibrar con un peso conocido
serial_bridge/            Bridge Python de USB serial a HTTPS
docs/                     Requisitos, arquitectura y montaje
```

## Componentes

- Arduino Uno
- Módulo HX711
- Celda de carga de 5 kg y cables
- Laptop con Python 3.14 para ejecutar el bridge

## Documentación

- [PRD](docs/PRD.md): requisitos y alcance del sensor y del bridge.
- [Arquitectura](docs/ARCHITECTURE.md): cableado, protocolo serial e integración.
- [Tareas](TASKS.md): estado y dependencias del trabajo.

No conectes el hardware ni envíes lecturas a producción hasta completar la
calibración y configurar `serial_bridge/.env` localmente. Ese archivo está
ignorado por Git; comparte únicamente una plantilla sin credenciales.
