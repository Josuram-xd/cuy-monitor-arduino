# Plataforma de pesaje en la jaula

Una plataforma baja frente al comedero, sobre la que los cuyes pasan solos varias veces al día. Pesa a la jaula
entera (los cuyes que estén encima en ese momento): la tendencia en el tiempo es lo que importa.

## Reglas de bienestar animal (no se negocian)

- **Firme:** no se mueve ni se inclina cuando un cuy sube; nada que lo asuste o lo haga resbalar.
- **Bordes redondeados** y sin tornillos ni puntas a la vista.
- **Cables y electrónica fuera de su alcance:** los cuyes roen todo. Ningún cable pasa por dentro de la jaula sin
  canaleta metálica; el HX711 y el Arduino van afuera, en una caja cerrada.
- **Baja:** como máximo 3 cm sobre el piso, con rampa suave si hace falta, para que suban sin saltar.
- Superficie **lavable y antideslizante**, sin pintura ni barniz tóxico.

Si algún cambio pone en riesgo a los animales, no se hace.

## Diseño

```
  vista lateral                                   vista superior
                                                  ┌──────────────────────────────┐
  ┌──────────────────────────────┐ ← tapa         │                              │
  │░░░░░░░ goma antideslizante ░░│   30×20 cm     │        tapa 30 × 20 cm       │
  └──────┬───────────────────────┘                │   ┌──────────────────┐       │
         │ celda (barra)  ◄── tornillo arriba     │   │  celda de carga  │       │
  ┌──────┴───────────────────────┐ ← base         │   └──────────────────┘       │
  │                              │   30×20 cm     │                              │
  └──────────────────────────────┘                └──────────────────────────────┘
     separadores de 5 mm arriba y abajo: la tapa solo se apoya en la celda
```

- La **celda tipo barra** va entre la base y la tapa, atornillada a la base por un extremo y a la tapa por el
  otro, con separadores de 5 mm para que la barra pueda flexionar.
- La tapa **no toca** las paredes de la jaula ni la base: si roza, parte del peso no se mide.
- Celda de **5 kg**: varios cuyes adultos juntos no pasan de ~4 kg.

## Materiales

| Material | Cantidad | Para qué |
|---|---|---|
| Celda de carga tipo barra 5 kg + módulo HX711 | 1 | Medir el peso |
| Arduino Uno + cable USB A-B (2 m o más) | 1 | Leer la celda y mandar a la laptop |
| Tablero MDF o acrílico de 6 mm, 30 × 20 cm | 2 | Base y tapa |
| Tornillos M4 o M5 con separadores de 5 mm | 4 | Fijar la celda a la base y a la tapa |
| Lámina de goma antideslizante, 30 × 20 cm | 1 | Superficie lavable sobre la tapa |
| Caja plástica con tapa | 1 | HX711 y Arduino, fuera de la jaula |
| Canaleta metálica | 1 m | Proteger el cable entre la celda y la caja |
| Lija y cinta de borde | — | Redondear y cubrir los bordes |

## Dónde ponerla

- **Frente al comedero**, donde pasan a diario: así hay lecturas estables seguido.
- Sobre piso plano; una base inclinada cambia la lectura.
- El cable sale de la jaula por el lado más corto hasta la caja, siempre dentro de la canaleta.

## Después de montarla

1. Calibra con la plataforma ya puesta ([calibration.md](calibration.md)); la tara quita el peso de la tapa.
2. Revisa unos días la gráfica de peso del dashboard (Task 7.3): con la jaula tranquila tiene que haber lecturas
   estables varias veces por hora.
3. Si casi nunca llega una lectura estable, se ajusta el umbral de estabilidad del firmware (Task 7.4).
