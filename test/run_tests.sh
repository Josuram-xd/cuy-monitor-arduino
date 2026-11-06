#!/usr/bin/env bash
# Compiles every sketch on the PC against the stubs in test/stubs and runs its test.
# It checks the logic and the code paths, not the timing of the real HX711: that is the Serial Monitor's job.
#
#   test/run_tests.sh
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$root/test/build"
mkdir -p "$build"
flags=(-std=c++17 -Wall -Wextra -Werror -Wno-unused-parameter -I "$root/test/stubs" -I "$root/test")
status=0

run() {
  local name="$1" sketch="$2" test="$3"
  # Arduino compiles a .ino as C++ with Arduino.h included first: the same here
  if g++ "${flags[@]}" -include Arduino.h -x c++ "$sketch" -x none "$test" "$root/test/stubs/host.cpp" \
      -o "$build/$name"; then
    "$build/$name" || status=1
  else
    echo "$name: does not compile"
    status=1
  fi
}

run calibrate "$root/calibration/calibrate/calibrate.ino" "$root/test/test_calibrate.cpp"

exit "$status"
