#include <Arduino.h>
#include <EEPROM.h>
#include <HX711.h>

namespace host {
unsigned long now_ms = 0;
std::string serial_input;
std::string serial_output;
long hx711_raw = 0;
bool hx711_ready = true;
uint8_t eeprom[1024];
}  // namespace host

HardwareSerial Serial;
EEPROMClass EEPROM;
