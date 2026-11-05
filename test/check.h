// A few lines of test framework: CHECK records a failure and keeps going, main returns how many failed.
#pragma once

#include <cmath>
#include <cstdio>
#include <string>

static int failures = 0;
static int checks = 0;

#define CHECK(condition)                                                   \
  do {                                                                     \
    ++checks;                                                              \
    if (!(condition)) {                                                    \
      ++failures;                                                          \
      std::printf("  FAILED %s:%d: %s\n", __FILE__, __LINE__, #condition); \
    }                                                                      \
  } while (0)

#define CHECK_NEAR(actual, expected, tolerance) CHECK(std::fabs((actual) - (expected)) <= (tolerance))

#define CHECK_CONTAINS(text, part) CHECK(std::string(text).find(part) != std::string::npos)

inline int report(const char *suite) {
  std::printf("%s: %d checks, %d failed\n", suite, checks, failures);
  return failures == 0 ? 0 : 1;
}
