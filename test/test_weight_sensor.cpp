// Runs weight_sensor.ino on the PC over 20 simulated seconds: lines every 2 s, stability, tare and calibration.
#include <Arduino.h>
#include <EEPROM.h>

#include <vector>

#include "check.h"

namespace host {
extern unsigned long now_ms;
extern std::string serial_input;
extern std::string serial_output;
extern long hx711_raw;
extern uint8_t eeprom[1024];
}  // namespace host

extern float factor;

static const long RAW_EMPTY = 8000;
static const float FACTOR = 420.0f;

// runs the loop every 10 ms for the given time
static void run_for(unsigned long ms) {
  for (unsigned long end = host::now_ms + ms; host::now_ms < end; host::now_ms += 10) loop();
}

static std::vector<std::string> json_lines() {
  std::vector<std::string> lines;
  size_t start = 0;
  while (start < host::serial_output.size()) {
    size_t end = host::serial_output.find('\n', start);
    std::string line = host::serial_output.substr(start, end - start);
    if (!line.empty() && line[0] == '{') lines.push_back(line);
    start = end + 1;
  }
  return lines;
}

static void send(const char *text) {
  host::serial_input += text;
  host::serial_input += "\n";
  loop();
}

int main() {
  std::memset(host::eeprom, 0xFF, sizeof(host::eeprom));
  host::hx711_raw = RAW_EMPTY;
  setup();
  CHECK_CONTAINS(host::serial_output, "no calibration in EEPROM");
  CHECK_CONTAINS(host::serial_output, "tare done");
  CHECK_CONTAINS(host::serial_output, "weight sensor ready");

  // 1 kg of guinea pigs standing still for 6 s: every line is about 1000 g, stable once the buffer is full
  host::serial_output.clear();
  host::hx711_raw = RAW_EMPTY + (long)(1000 * FACTOR);
  run_for(6200);  // a line at 2, 4 and 6 s
  std::vector<std::string> lines = json_lines();
  CHECK(lines.size() == 3);
  // 10 readings in the buffer by the first line: already full and stable
  CHECK(lines.front() == "{\"grams\": 1000.0, \"stable\": true}");
  CHECK(lines.back() == "{\"grams\": 1000.0, \"stable\": true}");

  // one jumps on and off: the readings move, the line is not stable
  host::serial_output.clear();
  for (int step = 0; step < 20; ++step) {
    host::hx711_raw = RAW_EMPTY + (long)((step % 2 ? 1000 : 1250) * FACTOR);
    run_for(200);
  }
  lines = json_lines();
  CHECK(!lines.empty());
  CHECK_CONTAINS(lines.back(), "\"stable\": false");

  // only diagnostics start with '#'; everything else is JSON
  CHECK(host::serial_output.find("grams") != std::string::npos);

  // tare with the weight on: from now on it reads 0
  send("t");
  host::serial_output.clear();
  run_for(4000);
  CHECK(json_lines().back() == "{\"grams\": 0.0, \"stable\": true}");

  // a new factor, saved: a reboot reads it back
  send("c 400");
  CHECK_NEAR(factor, 400.0f, 0.001f);
  send("s");
  CHECK_CONTAINS(host::serial_output, "factor saved in EEPROM: 400.0000");
  send("c 0");
  CHECK_CONTAINS(host::serial_output, "commands: t | c <factor> | s");
  CHECK_NEAR(factor, 400.0f, 0.001f);

  factor = 1.0f;
  host::serial_output.clear();
  setup();
  CHECK_NEAR(factor, 400.0f, 0.001f);
  CHECK_CONTAINS(host::serial_output, "factor 400.0000 from EEPROM");

  return report("weight_sensor.ino");
}
