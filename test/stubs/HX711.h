// Host stand-in for the HX711 library (bogde): the same methods, over a raw value the test sets.
#pragma once

#include <Arduino.h>

namespace host {
extern long hx711_raw;    // what the load cell reads, in raw units
extern bool hx711_ready;
}  // namespace host

class HX711 {
 public:
  void begin(byte, byte, byte = 128) {}
  bool is_ready() { return host::hx711_ready; }
  long read() { return host::hx711_raw; }
  double read_average(byte = 10) { return static_cast<double>(host::hx711_raw); }
  double get_value(byte times = 1) { return read_average(times) - offset_; }
  float get_units(byte times = 1) { return static_cast<float>(get_value(times) / scale_); }
  void tare(byte times = 10) { offset_ = static_cast<long>(read_average(times)); }
  void set_scale(float scale = 1.f) { scale_ = scale; }
  float get_scale() { return scale_; }
  void set_offset(long offset = 0) { offset_ = offset; }
  long get_offset() { return offset_; }

 private:
  long offset_ = 0;
  float scale_ = 1.f;
};
