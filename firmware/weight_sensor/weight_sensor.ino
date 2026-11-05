// The cage scale: reads the load cell through the HX711 every 200 ms and prints one JSON line every 2 s,
// {"grams": 812.4, "stable": true}, at 115200 baud for serial_bridge (Task 4). Diagnostics start with '#'.
// Commands: "t" tare · "c <factor>" calibration factor · "s" save the factor in EEPROM.

#include <EEPROM.h>
#include <HX711.h>

#include "weight_logic.h"

// docs/wiring.md
const uint8_t PIN_DOUT = 3;
const uint8_t PIN_SCK = 2;

// Same EEPROM layout as calibration/calibrate/calibrate.ino: keep both in sync
const int CALIBRATION_ADDRESS = 0;
const uint32_t CALIBRATION_MAGIC = 0x43555931UL;  // "CUY1"
// only until the cell is calibrated: the readings will be wrong, and the start says so
const float DEFAULT_FACTOR = 420.0f;

struct StoredCalibration {
  uint32_t magic;
  float factor;
};

const unsigned long READ_INTERVAL_MS = 200;
const unsigned long PRINT_INTERVAL_MS = 2000;
const uint8_t TARE_SAMPLES = 10;

HX711 scale;
weight::RingBuffer buffer;
weight::LineReader commands;
float factor = DEFAULT_FACTOR;
unsigned long lastRead = 0;
unsigned long lastPrint = 0;

void loadFactor() {
  StoredCalibration stored;
  EEPROM.get(CALIBRATION_ADDRESS, stored);
  if (stored.magic == CALIBRATION_MAGIC && stored.factor != 0.0f) {
    factor = stored.factor;
    Serial.print(F("# factor "));
    Serial.print(factor, 4);
    Serial.println(F(" from EEPROM"));
  } else {
    factor = DEFAULT_FACTOR;
    Serial.println(F("# no calibration in EEPROM: run calibrate.ino (docs/calibration.md)"));
  }
  scale.set_scale(factor);
}

void saveFactor() {
  StoredCalibration stored = {CALIBRATION_MAGIC, factor};
  EEPROM.put(CALIBRATION_ADDRESS, stored);
  Serial.print(F("# factor saved in EEPROM: "));
  Serial.println(factor, 4);
}

void tare() {
  scale.tare(TARE_SAMPLES);
  buffer.clear();  // readings taken before the tare would make the next line wrong
  Serial.println(F("# tare done"));
}

void runCommand(const char *line) {
  weight::Command command = weight::parseCommand(line);
  switch (command.type) {
    case weight::COMMAND_TARE:
      tare();
      break;
    case weight::COMMAND_SET_FACTOR:
      factor = command.value;
      scale.set_scale(factor);
      buffer.clear();
      Serial.print(F("# factor set to "));
      Serial.print(factor, 4);
      Serial.println(F(" (send s to keep it)"));
      break;
    case weight::COMMAND_SAVE:
      saveFactor();
      break;
    default:
      Serial.println(F("# commands: t | c <factor> | s"));
  }
}

void printReading() {
  char line[weight::LINE_LENGTH];
  weight::formatReading(line, sizeof(line), weight::summarize(buffer));
  Serial.println(line);
}

void setup() {
  Serial.begin(115200);
  scale.begin(PIN_DOUT, PIN_SCK);
  loadFactor();
  tare();
  Serial.println(F("# weight sensor ready"));
}

void loop() {
  while (Serial.available() > 0) {
    if (commands.feed((char)Serial.read())) {
      runCommand(commands.line());
    }
  }

  unsigned long now = millis();
  if (now - lastRead >= READ_INTERVAL_MS && scale.is_ready()) {
    lastRead = now;
    buffer.push(scale.get_units(1));
  }
  if (now - lastPrint >= PRINT_INTERVAL_MS && buffer.count() > 0) {
    lastPrint = now;
    printReading();
  }
}
