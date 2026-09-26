# PRD — Sensor de peso (cuy-monitor-arduino)

> PRD del componente. El PRD general del producto está en `cuy-monitor-backend/docs/PRD.md`.
> Dueño: compañero · Última revisión: 26 de septiembre de 2026

## 1. Qué es

Una plataforma de pesaje dentro de la jaula (celda de carga + HX711 + Arduino Uno) y un programa en la laptop (`serial_bridge`) que manda cada lectura al backend. El peso es **una señal más de salud**: si el peso de la jaula baja de forma sostenida, puede indicar que los cuyes están comiendo menos.

**Limitación aceptada:** en esta versión la lectura es **de la jaula**, no de un cuy específico (no sabemos quién está parado en la plataforma). Asociar el peso a cada cuy queda como mejora futura (cruzándolo con la cámara).

## 2. Usuarios

- **Backend:** recibe las lecturas.
- **Equipo técnico:** arma, calibra y monta la plataforma.
- **Criador (indirecto):** ve la gráfica de peso en el dashboard.

## 3. Requisitos funcionales

| ID | Requisito |
|---|---|
| W-01 | Leer la celda de carga con el HX711 y convertir a gramos con un factor de calibración |
| W-02 | Promediar varias lecturas para bajar el ruido |
| W-03 | Detectar si la lectura es **estable** (el animal está quieto sobre la plataforma) |
| W-04 | Mandar por USB serial una línea JSON cada ~2 s: `{"grams": 812.4, "stable": true}` |
| W-05 | Poder hacer tara (poner en cero) con un comando por serial, sin volver a cargar el sketch |
| W-06 | Sketch aparte para calibrar con un peso conocido |
| W-07 | `serial_bridge`: leer el puerto serial y mandar las lecturas estables a `POST /api/ingestion/events` (tipo `WEIGHT`) con `X-API-Key` |
| W-08 | `serial_bridge`: si se cae internet, guardar las lecturas en un buffer limitado y reenviarlas después |
| W-09 | `serial_bridge`: reconectarse solo si se desconecta el USB |

## 4. Requisitos no funcionales

| Qué | Meta |
|---|---|
| Rango | 0–5 kg (un cuy adulto pesa ~0.7–1.2 kg) |
| Precisión | ± 5 g después de calibrar |
| Frecuencia al backend | Máximo 1 lectura estable cada 10 s |
| Disponibilidad | El bridge arranca solo al encender la laptop |
| Bienestar animal | Plataforma firme, sin bordes filosos, sin cables al alcance de los cuyes, sin moverse al pisarla |
| Costo | Solo el kit (Arduino Uno, HX711, celda de carga de 5 kg, cables) |

## 5. Entregas

- **Avance (30 sept):** no entra. El backend puede probar la ingesta con lecturas falsas.
- **Octubre:** firmware + calibración + bridge funcionando contra el backend desplegado.
- **Prueba en jaula (27 oct – 9 nov):** plataforma montada, varios días de lecturas.

## 6. Fuera de alcance

- Identificar qué cuy está sobre la plataforma.
- WiFi en el Arduino (Uno no tiene; un ESP32 es mejora futura y eliminaría el bridge).
- Mostrar el peso en una pantalla física.

## 7. Dependencias

| De | Qué necesita |
|---|---|
| `cuy-monitor-backend` | Endpoint `POST /api/ingestion/events`, sobre común y payload `WEIGHT`, la `API_KEY` |
| Laptop del criadero | Python 3.14, puerto USB libre, internet |
