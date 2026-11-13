// Hardware smoke test: checks that the Arduino, the HX711 and the load cell are
// alive before the real firmware exists. Prints only diagnostic ('#') lines, at
// the same 115200 baud as the serial protocol, so the bridge would ignore them.

#include <HX711.h>

const uint8_t PIN_DT = 3;
const uint8_t PIN_SCK = 2;
const unsigned long PRINT_INTERVAL_MS = 500;
const unsigned long HX711_TIMEOUT_MS = 2000;
const uint8_t DEAD_READS_WARN = 10;

HX711 scale;
unsigned long last_print = 0;
unsigned long last_ready = 0;
uint8_t dead_reads = 0;

void setup() {
  Serial.begin(115200);
  scale.begin(PIN_DT, PIN_SCK);
  Serial.println(F("# hw_test ready, send 't' to tare"));
  last_ready = millis();
}

void loop() {
  if (Serial.read() == 't') {
    scale.tare(10);
    Serial.println(F("# tare done"));
  }

  unsigned long now = millis();
  if (now - last_print < PRINT_INTERVAL_MS) return;
  last_print = now;

  if (scale.is_ready()) {
    last_ready = now;
    long raw = scale.read();
    Serial.print(F("# raw="));
    Serial.println(raw);
    // a floating DT line reads LOW, so is_ready() lies and every read is 0 or -1
    if (raw == 0 || raw == -1) {
      if (++dead_reads == DEAD_READS_WARN) {
        Serial.println(F("# raw stuck, HX711 sends nothing: check DT=D3, VCC=5V, GND"));
      }
    } else {
      dead_reads = 0;
    }
  } else if (now - last_ready >= HX711_TIMEOUT_MS) {
    last_ready = now;  // warn once per timeout window, not every loop
    Serial.println(F("# HX711 not responding: check DT=D3, SCK=D2, VCC=5V, GND"));
  }
}
