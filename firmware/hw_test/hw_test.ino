#include <HX711.h>

const byte PIN_DT = 3;
const byte PIN_SCK = 2;

HX711 balanza;

void setup() {
  Serial.begin(115200);
  balanza.begin(PIN_DT, PIN_SCK);
  Serial.println("Prueba de bascula iniciada");
}

void loop() {
  if (balanza.wait_ready_timeout(1000)) {
    long lectura = balanza.read();
    Serial.print("Lectura: ");
    Serial.println(lectura);
  } else {
    Serial.println("El HX711 no responde. Revisa DT, SCK, 5V y GND.");
  }

  delay(500);
}