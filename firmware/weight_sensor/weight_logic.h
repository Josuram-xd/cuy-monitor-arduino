// The logic of the weight sensor, apart from the hardware so it can be tested on a PC (test/run_tests.sh).
// Written for the Uno: no dynamic memory, no String, no float printf (avr-libc's snprintf has no %f).
#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace weight {

// ~2 s of readings at one every 200 ms (docs/ARCHITECTURE.md, section 4)
const uint8_t BUFFER_SIZE = 10;
// the buffer is stable when its readings are no further apart than this
const float STABLE_DELTA_G = 5.0f;
// below this the platform is not just empty: usually a tare problem, never stable
const float NEGATIVE_LIMIT_G = -20.0f;
const uint8_t COMMAND_LENGTH = 24;
const uint8_t LINE_LENGTH = 48;

// The last BUFFER_SIZE readings; the oldest is replaced by the newest.
class RingBuffer {
 public:
  void push(float grams) {
    values_[next_] = grams;
    next_ = (next_ + 1) % BUFFER_SIZE;
    if (count_ < BUFFER_SIZE) {
      ++count_;
    }
  }

  void clear() {
    next_ = 0;
    count_ = 0;
  }

  uint8_t count() const { return count_; }

  bool full() const { return count_ == BUFFER_SIZE; }

  float mean() const {
    if (count_ == 0) {
      return 0.0f;
    }
    float sum = 0.0f;
    for (uint8_t i = 0; i < count_; ++i) {
      sum += values_[i];
    }
    return sum / count_;
  }

  // max - min of the readings in the buffer
  float spread() const {
    if (count_ == 0) {
      return 0.0f;
    }
    float low = values_[0];
    float high = values_[0];
    for (uint8_t i = 1; i < count_; ++i) {
      if (values_[i] < low) low = values_[i];
      if (values_[i] > high) high = values_[i];
    }
    return high - low;
  }

 private:
  float values_[BUFFER_SIZE];
  uint8_t next_ = 0;
  uint8_t count_ = 0;
};

struct Reading {
  float grams;
  bool stable;
};

// What gets printed for the buffer: its mean, stable only when it is full and settled.
// Grams are never negative (the contract asks for >= 0): a little below 0 is an empty platform;
// far below 0 is a tare problem, sent as 0 and not stable so the bridge drops it.
inline Reading summarize(const RingBuffer &buffer) {
  Reading reading;
  reading.grams = buffer.mean();
  reading.stable = buffer.full() && buffer.spread() <= STABLE_DELTA_G;
  if (reading.grams < NEGATIVE_LIMIT_G) {
    reading.stable = false;
  }
  if (reading.grams < 0.0f) {
    reading.grams = 0.0f;
  }
  return reading;
}

// {"grams": 812.4, "stable": true} — one decimal, the serial protocol of docs/ARCHITECTURE.md.
inline int formatReading(char *out, size_t size, const Reading &reading) {
  long tenths = (long)(reading.grams * 10.0f + 0.5f);
  return snprintf(out, size, "{\"grams\": %ld.%ld, \"stable\": %s}", tenths / 10, tenths % 10,
                  reading.stable ? "true" : "false");
}

enum CommandType { COMMAND_TARE, COMMAND_SET_FACTOR, COMMAND_SAVE, COMMAND_UNKNOWN };

struct Command {
  CommandType type;
  float value;
};

// "t" tare · "c <factor>" calibration factor · "s" save it in EEPROM
inline Command parseCommand(const char *line) {
  Command command = {COMMAND_UNKNOWN, 0.0f};
  if (strcmp(line, "t") == 0) {
    command.type = COMMAND_TARE;
  } else if (strcmp(line, "s") == 0) {
    command.type = COMMAND_SAVE;
  } else if (line[0] == 'c' && line[1] == ' ') {
    char *end;
    float factor = (float)strtod(line + 2, &end);
    if (end != line + 2 && factor != 0.0f) {
      command.type = COMMAND_SET_FACTOR;
      command.value = factor;
    }
  }
  return command;
}

// Collects the characters of the serial port into lines; too long a line is cut, never overflows.
class LineReader {
 public:
  // true when c ended a line that is not empty; the line is then in line()
  bool feed(char c) {
    if (c == '\r') {
      return false;
    }
    if (c == '\n') {
      line_[length_] = '\0';
      bool complete = length_ > 0;
      length_ = 0;
      return complete;
    }
    if (length_ < COMMAND_LENGTH - 1) {
      line_[length_++] = c;
    }
    return false;
  }

  const char *line() const { return line_; }

 private:
  char line_[COMMAND_LENGTH] = {0};
  uint8_t length_ = 0;
};

}  // namespace weight
