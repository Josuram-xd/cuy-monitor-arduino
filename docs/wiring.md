# Cableado: celda de carga → HX711 → Arduino Uno

Los pines no se cambian sin actualizar también los dos sketches (`PIN_DOUT` y `PIN_SCK`).

## Diagrama

```
   Celda de carga 5 kg                HX711                          Arduino Uno
  ┌────────────────────┐        ┌───────────────┐               ┌──────────────────┐
  │ rojo    (E+) ──────┼────────┤ E+        VCC ├───────────────┤ 5V               │
  │ negro   (E−) ──────┼────────┤ E−        GND ├───────────────┤ GND              │
  │ blanco  (A−) ──────┼────────┤ A−        DT  ├───────────────┤ D3   (PIN_DOUT)  │
  │ verde   (A+) ──────┼────────┤ A+        SCK ├───────────────┤ D2   (PIN_SCK)   │
  └────────────────────┘        └───────────────┘               │ USB ──► laptop   │
                                                                └──────────────────┘
```

| HX711 | Arduino Uno | Para qué |
|---|---|---|
| VCC | 5V | Alimentación |
| GND | GND | Tierra común |
| DT (DOUT) | D3 | Datos de la celda |
| SCK | D2 | Reloj |

| Cable de la celda | HX711 |
|---|---|
| Rojo | E+ (excitación +) |
| Negro | E− (excitación −) |
| Blanco | A− (señal −) |
| Verde | A+ (señal +) |

Canal A, ganancia 128 (lo que usa la librería HX711 de bogde por defecto).

## Cuidado

- **Los colores de la celda varían por fabricante.** Revisa su hoja de datos. Si al poner peso la lectura baja en
  vez de subir, A+ y A− están al revés: inviértelos.
- El Arduino se alimenta por el mismo USB que va a la laptop: no hace falta otra fuente.
- Cables de la celda al HX711 **cortos** (menos de 30 cm) y lejos de cargadores; el ruido se nota como lecturas
  que nunca quedan estables.
- Suelda o usa borneras: un cable suelto da lecturas que saltan, y el bridge las descarta como basura.
- El HX711 y el Arduino van **fuera** del alcance de los cuyes, en una caja cerrada ([mounting.md](mounting.md)).

## Comprobar el cableado

1. Sube `calibration/calibrate/calibrate.ino` y abre el Serial Monitor a 115200.
2. Envía `t` con la plataforma vacía; luego apoya la mano: `# reading` tiene que subir y volver al soltar.
3. Si no cambia nada, revisa DT/SCK; si cambia al revés, A+/A−. Después sigue con [calibration.md](calibration.md).

## Fotos

Van aquí al armar el circuito real (Task 2.2): el HX711 con sus cuatro cables, el Arduino y la caja cerrada.
