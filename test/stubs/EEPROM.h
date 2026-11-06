// Host stand-in for the EEPROM library: 1 KB like the Uno, starting erased (0xFF), kept between "reboots".
#pragma once

#include <Arduino.h>

namespace host {
extern uint8_t eeprom[1024];
}  // namespace host

class EEPROMClass {
 public:
  template <typename T>
  T &get(int address, T &value) {
    memcpy(&value, host::eeprom + address, sizeof(T));
    return value;
  }
  template <typename T>
  const T &put(int address, const T &value) {
    memcpy(host::eeprom + address, &value, sizeof(T));
    return value;
  }
  uint8_t read(int address) { return host::eeprom[address]; }
  void write(int address, uint8_t value) { host::eeprom[address] = value; }
};

extern EEPROMClass EEPROM;
