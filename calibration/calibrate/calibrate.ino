// Finds the calibration factor of the load cell with a known weight and stores it in EEPROM,
// where weight_sensor.ino reads it on start (Task 3).
//
// Serial Monitor at 115200 baud, line ending "Newline". Commands:
//   t            tare: the empty platform becomes 0 g
//   w <grams>    a known weight is on the platform: compute the factor from it (e.g. "w 500")
//   c <factor>   set the factor by hand, to fine-tune it
//   s            save the factor in EEPROM
// Every second it prints the current reading, so the factor can be checked with a second weight.

#include <EEPROM.h>
#include <HX711.h>

// Same pins as weight_sensor.ino and docs/wiring.md
const uint8_t PIN_DOUT = 3;
const uint8_t PIN_SCK = 2;

// Same EEPROM layout as weight_sensor.ino: keep both in sync
const int CALIBRATION_ADDRESS = 0;
const uint32_t CALIBRATION_MAGIC = 0x43555931UL;  // "CUY1": the factor was written by us

struct StoredCalibration {
  uint32_t magic;
  float factor;
};

const uint8_t SAMPLES_PER_MEASURE = 20;
const unsigned long PRINT_INTERVAL_MS = 1000;
const uint8_t COMMAND_LENGTH = 24;

HX711 scale;
float factor = 1.0f;
unsigned long lastPrint = 0;
char command[COMMAND_LENGTH];
uint8_t commandLength = 0;

void loadFactor() {
  StoredCalibration stored;
  EEPROM.get(CALIBRATION_ADDRESS, stored);
  if (stored.magic == CALIBRATION_MAGIC && stored.factor != 0.0f) {
    factor = stored.factor;
    Serial.print(F("# factor read from EEPROM: "));
    Serial.println(factor, 4);
  } else {
    Serial.println(F("# no factor in EEPROM yet"));
  }
  scale.set_scale(factor);
}

void tare() {
  scale.tare(SAMPLES_PER_MEASURE);
  Serial.println(F("# tare done: the platform reads 0 g now"));
}

void calibrateWith(float knownGrams) {
  if (knownGrams <= 0.0f) {
    Serial.println(F("# the known weight must be more than 0 g"));
    return;
  }
  // get_value() is the raw reading minus the tare; divided by the grams it gives the factor
  float raw = scale.get_value(SAMPLES_PER_MEASURE);
  if (raw == 0.0f) {
    Serial.println(F("# the reading did not change: check the wiring and tare first"));
    return;
  }
  factor = raw / knownGrams;
  scale.set_scale(factor);
  Serial.print(F("# factor: "));
  Serial.println(factor, 4);
  Serial.println(F("# check it with another weight, then send s to save it"));
}

void setFactor(float newFactor) {
  if (newFactor == 0.0f) {
    Serial.println(F("# the factor can't be 0"));
    return;
  }
  factor = newFactor;
  scale.set_scale(factor);
  Serial.print(F("# factor set to "));
  Serial.println(factor, 4);
}

void saveFactor() {
  StoredCalibration stored = {CALIBRATION_MAGIC, factor};
  EEPROM.put(CALIBRATION_ADDRESS, stored);
  Serial.print(F("# factor saved in EEPROM: "));
  Serial.println(factor, 4);
}

void runCommand(const char *line) {
  switch (line[0]) {
    case 't':
      tare();
      break;
    case 'w':
      calibrateWith(atof(line + 1));
      break;
    case 'c':
      setFactor(atof(line + 1));
      break;
    case 's':
      saveFactor();
      break;
    default:
      Serial.println(F("# commands: t | w <grams> | c <factor> | s"));
  }
}

void readCommands() {
  while (Serial.available() > 0) {
    char received = (char)Serial.read();
    if (received == '\r') {
      continue;
    }
    if (received == '\n') {
      command[commandLength] = '\0';
      if (commandLength > 0) {
        runCommand(command);
      }
      commandLength = 0;
    } else if (commandLength < COMMAND_LENGTH - 1) {
      command[commandLength++] = received;
    }
  }
}

void setup() {
  Serial.begin(115200);
  scale.begin(PIN_DOUT, PIN_SCK);
  Serial.println(F("# calibration: empty the platform and send t"));
  loadFactor();
}

void loop() {
  readCommands();
  unsigned long now = millis();
  if (now - lastPrint >= PRINT_INTERVAL_MS && scale.is_ready()) {
    lastPrint = now;
    Serial.print(F("# reading: "));
    Serial.print(scale.get_units(5), 1);
    Serial.print(F(" g (factor "));
    Serial.print(factor, 4);
    Serial.println(F(")"));
  }
}
