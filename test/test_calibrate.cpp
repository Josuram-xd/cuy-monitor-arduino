// Runs calibrate.ino on the PC: tare, a known weight, the factor, saving it and reading it after a reboot.
#include <Arduino.h>
#include <EEPROM.h>

#include "check.h"

namespace host {
extern unsigned long now_ms;
extern std::string serial_input;
extern std::string serial_output;
extern long hx711_raw;
extern uint8_t eeprom[1024];
}  // namespace host

extern float factor;

static void send(const char *line) {
  host::serial_input += line;
  host::serial_input += "\n";
  loop();
}

int main() {
  std::memset(host::eeprom, 0xFF, sizeof(host::eeprom));

  setup();
  CHECK_CONTAINS(host::serial_output, "no factor in EEPROM yet");

  host::hx711_raw = 8000;  // the empty platform
  send("t");
  CHECK_CONTAINS(host::serial_output, "tare done");

  host::hx711_raw = 8000 + 211500;  // a 500 g bag: 423 raw units per gram
  send("w 500");
  CHECK_NEAR(factor, 423.0f, 0.01f);
  CHECK_CONTAINS(host::serial_output, "factor: 423.0000");

  host::serial_output.clear();
  host::now_ms = 1000;
  loop();
  CHECK_CONTAINS(host::serial_output, "reading: 500.0 g");

  send("w 0");
  CHECK_CONTAINS(host::serial_output, "must be more than 0 g");
  CHECK_NEAR(factor, 423.0f, 0.01f);

  send("c 420.5");
  CHECK_NEAR(factor, 420.5f, 0.01f);
  send("s");
  CHECK_CONTAINS(host::serial_output, "factor saved in EEPROM: 420.5000");

  send("x");
  CHECK_CONTAINS(host::serial_output, "commands: t | w <grams> | c <factor> | s");

  // a power cut: the factor comes back from EEPROM
  factor = 1.0f;
  host::serial_output.clear();
  setup();
  CHECK_NEAR(factor, 420.5f, 0.01f);
  CHECK_CONTAINS(host::serial_output, "factor read from EEPROM: 420.5000");

  return report("calibrate.ino");
}
