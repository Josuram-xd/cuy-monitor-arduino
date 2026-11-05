# Calibración de la celda de carga

La celda entrega un número crudo; el **factor de calibración** lo convierte en gramos (`gramos = crudo / factor`).
Cada celda tiene el suyo, así que se calibra una vez al montarla y se guarda en la EEPROM del Arduino: un corte de
luz no lo borra y `weight_sensor.ino` lo lee al arrancar.

## Qué se necesita

- La plataforma ya armada sobre la celda (el peso de la plataforma se quita con la tara).
- Dos pesos conocidos, pesados antes en una balanza de cocina: por ejemplo una botella de agua de ~500 g y un
  paquete de ~1000 g. Anota su peso exacto.

## Pasos

1. Sube `calibration/calibrate/calibrate.ino` con Arduino IDE 2 (o `arduino-cli upload`).
2. Abre el **Serial Monitor** a **115200** baudios, con el fin de línea en **Newline**.
3. Con la plataforma **vacía**, envía `t`. Debe responder `# tare done`.
4. Pon el primer peso conocido en el centro y envía `w <gramos>`, por ejemplo `w 512`.
   Responde `# factor: 423.1234`.
5. Cada segundo imprime `# reading: ... g`: debe mostrar el peso que pusiste.
6. Cambia al **segundo** peso conocido y compara. Si se aleja más de ±5 g, ajusta a mano con `c <factor>`
   (factor más alto = gramos más bajos) hasta que los dos pesos den bien.
7. Envía `s` para guardarlo. Responde `# factor saved in EEPROM: ...`.
8. Sube `firmware/weight_sensor/weight_sensor.ino`: al arrancar imprime `# factor ... from EEPROM`.

| Comando | Qué hace |
|---|---|
| `t` | Tara: la plataforma vacía pasa a ser 0 g |
| `w <gramos>` | Calcula el factor con el peso conocido que está encima |
| `c <factor>` | Pone el factor a mano, para afinarlo |
| `s` | Guarda el factor en la EEPROM |

## Error medido

Meta: **±5 g** (ARCHITECTURE, sección 5). Se llena al calibrar la celda real (Task 3.3):

| Fecha | Factor guardado | Peso conocido (g) | Lectura (g) | Error (g) |
|---|---|---|---|---|
| | | | | |
| | | | | |

## Si algo no cuadra

- **La lectura no cambia al poner peso:** revisa DT → D3 y SCK → D2, y los cuatro cables de la celda al HX711
  ([wiring.md](wiring.md)).
- **Sale negativo al poner peso:** los cables A+ y A− están al revés; inviértelos (o usa un factor negativo).
- **La lectura deriva sola:** espera unos minutos con todo encendido antes de tarar; la celda se estabiliza con la
  temperatura.
