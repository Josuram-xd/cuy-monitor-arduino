// Host stand-in for the Arduino core: just enough to compile and run the sketches on a PC.
// Serial reads from a scripted input and records what the sketch prints; millis() is a clock the test moves.
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

typedef uint8_t byte;

class __FlashStringHelper;
#define F(text) (reinterpret_cast<const __FlashStringHelper *>(text))

namespace host {
extern unsigned long now_ms;
extern std::string serial_input;
extern std::string serial_output;
}  // namespace host

inline unsigned long millis() { return host::now_ms; }

class HardwareSerial {
 public:
  void begin(unsigned long) {}
  int available() { return static_cast<int>(host::serial_input.size()); }
  int read() {
    if (host::serial_input.empty()) return -1;
    char c = host::serial_input[0];
    host::serial_input.erase(0, 1);
    return c;
  }
  void print(const char *text) { host::serial_output += text; }
  void print(const __FlashStringHelper *text) { print(reinterpret_cast<const char *>(text)); }
  void print(char c) { host::serial_output += c; }
  void print(long value) { host::serial_output += std::to_string(value); }
  void print(int value) { print(static_cast<long>(value)); }
  void print(unsigned long value) { host::serial_output += std::to_string(value); }
  void print(double value, int digits = 2) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.*f", digits, value);
    host::serial_output += buffer;
  }
  template <typename T>
  void println(T value) {
    print(value);
    host::serial_output += '\n';
  }
  void println(double value, int digits) {
    print(value, digits);
    host::serial_output += '\n';
  }
  void println() { host::serial_output += '\n'; }
};

extern HardwareSerial Serial;

void setup();
void loop();
