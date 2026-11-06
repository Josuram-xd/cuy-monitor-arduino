// Unit tests of firmware/weight_sensor/weight_logic.h: the ring buffer, stability, the JSON line and commands.
#include "check.h"
#include "weight_logic.h"

using namespace weight;

static void fill(RingBuffer &buffer, float grams, uint8_t times) {
  for (uint8_t i = 0; i < times; ++i) buffer.push(grams);
}

static std::string line(float grams, bool stable) {
  char out[LINE_LENGTH];
  formatReading(out, sizeof(out), Reading{grams, stable});
  return out;
}

int main() {
  RingBuffer buffer;
  CHECK(buffer.count() == 0 && !buffer.full());
  CHECK(buffer.mean() == 0.0f);

  fill(buffer, 800.0f, BUFFER_SIZE);
  CHECK(buffer.full());
  CHECK_NEAR(buffer.mean(), 800.0f, 0.001f);
  CHECK(summarize(buffer).stable);

  // the ring keeps only the last BUFFER_SIZE readings: ten new ones replace the old mean
  fill(buffer, 1200.0f, BUFFER_SIZE);
  CHECK_NEAR(buffer.mean(), 1200.0f, 0.001f);
  CHECK(buffer.count() == BUFFER_SIZE);

  // a guinea pig stepping on: readings 60 g apart are not stable
  buffer.push(1260.0f);
  CHECK_NEAR(buffer.spread(), 60.0f, 0.001f);
  CHECK(!summarize(buffer).stable);

  // small noise, within STABLE_DELTA_G, is stable
  buffer.clear();
  for (uint8_t i = 0; i < BUFFER_SIZE; ++i) buffer.push(500.0f + (i % 2 ? 2.0f : -2.0f));
  CHECK(summarize(buffer).stable);

  // half full is never stable, whatever the values
  buffer.clear();
  fill(buffer, 500.0f, BUFFER_SIZE / 2);
  CHECK(!summarize(buffer).stable);

  // a little below zero is an empty platform: 0 g and still stable
  buffer.clear();
  fill(buffer, -3.0f, BUFFER_SIZE);
  CHECK(summarize(buffer).grams == 0.0f);
  CHECK(summarize(buffer).stable);

  // far below zero is a tare problem: 0 g and not stable
  buffer.clear();
  fill(buffer, -150.0f, BUFFER_SIZE);
  CHECK(summarize(buffer).grams == 0.0f);
  CHECK(!summarize(buffer).stable);

  CHECK(line(812.44f, true) == "{\"grams\": 812.4, \"stable\": true}");
  CHECK(line(812.46f, false) == "{\"grams\": 812.5, \"stable\": false}");
  CHECK(line(0.0f, true) == "{\"grams\": 0.0, \"stable\": true}");
  CHECK(line(4999.95f, true) == "{\"grams\": 5000.0, \"stable\": true}");

  CHECK(parseCommand("t").type == COMMAND_TARE);
  CHECK(parseCommand("s").type == COMMAND_SAVE);
  Command factor = parseCommand("c 423.25");
  CHECK(factor.type == COMMAND_SET_FACTOR);
  CHECK_NEAR(factor.value, 423.25f, 0.001f);
  CHECK(parseCommand("c 0").type == COMMAND_UNKNOWN);
  CHECK(parseCommand("c abc").type == COMMAND_UNKNOWN);
  CHECK(parseCommand("c").type == COMMAND_UNKNOWN);
  CHECK(parseCommand("tare").type == COMMAND_UNKNOWN);

  LineReader reader;
  const char *input = "t\r\n";
  bool complete = false;
  for (const char *c = input; *c; ++c) complete = reader.feed(*c);
  CHECK(complete && std::string(reader.line()) == "t");
  CHECK(!reader.feed('\n'));  // an empty line is not a command

  // a line longer than the buffer is cut, never written past its end
  for (int i = 0; i < 100; ++i) reader.feed('x');
  CHECK(reader.feed('\n'));
  CHECK(std::string(reader.line()).size() == COMMAND_LENGTH - 1);

  return report("weight_logic.h");
}
