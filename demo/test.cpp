/*
  Copyright 2021 Marcel Krueger

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
  OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "hidapi.hpp"
#include "x50q.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <exception>
#include <fmt/format.h>
#include <optional>
#include <span>
#include <thread>
#include <vector>

using namespace mfk;

/*
void set_profile(X50Q &dev) {
  using namespace std::chrono_literals;
  std::array<ByteSeconds, 144> durations = {
      3s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s,
      1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s,
      1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s,
      1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s,
      1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s,
      1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s,
      1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s, 1s};

  dev.apply_active_duration(durations);

  // Active effect
  std::array<X50Q::Effect, 144> effects;
  for (auto &e : effects)
    e = X50Q::Effect::Cycle;
  dev.apply_effects_active(effects);

  // These somehow set the color after pressing the key, set to all zero to make dark
  // dev.apply_colors_active(buffer);

  // Idle effect
  for (auto &e : effects)
    e = X50Q::Effect::SetColor;
  dev.apply_effects_idle(effects);
  return;

  // 4*11+8+8+13+7+10+17
  // 107 keys + two side bars with 2 LEDS each + four LED's for the volume knob = 115
  // Stride: 144. The other 29 positions don't seem to have any function.
  // The remaining 29 keys are mixed in.
  std::uint8_t colors[3][144] = {
      {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
  colors[0][0] = colors[1][0] = colors[2][0] = 0xff; // Make F5 white

  dev.apply_colors_idle(colors);
}
*/

std::array<std::uint8_t, 115> mapping = {
    45, 27,  18,  9,   14,  0,   5,   90, 95, 77,  63,  68,  54,  59,  99,  104, 111, 120, 129, 46,
    37, 28,  19,  10,  15,  1,   6,   91, 96, 81,  72,  64,  60,  61,  105, 106, 102, 110, 128, 119,
    47, 38,  29,  20,  11,  16,  2,   92, 82, 87,  73,  65,  69,  56,  101, 103, 100, 142, 115, 133,
    48, 39,  30,  21,  25,  12,  3,   93, 83, 84,  74,  66,  58,  139, 112, 130, 124, 49,  31,  33,
    24, 22,  13,  4,   94,  85,  86,  76, 70, 135, 136, 109, 127, 50,  41,  32,  23,  88,  79,  78,
    67, 140, 137, 138, 141, 132, 121, 36, 42, 123, 51,  117, 126, 118, 131,
};

// UK layout support
std::array<std::uint8_t, 117> ukMapping = {
    45, 27,  18,  9,   14,  0,   5,   90, 95, 77,  63,  68,  54,  59,  99,  104, 111, 120, 129, 46,
    37, 28,  19,  10,  15,  1,   6,   91, 96, 81,  72,  64,  60,  61,  105, 106, 102, 110, 128, 119,
    47, 38,  29,  20,  11,  16,  2,   92, 82, 87,  73,  65,  69,  56,  101, 103, 100, 142, 115, 133,
    48, 39,  30,  21,  25,  12,  3,   93, 83, 84,  74,  66,  57, 58,  139, 112, 130, 124, 49,  40, 31, 33,
    24, 22,  13,  4,   94,  85,  86,  76, 70, 135, 136, 109, 127, 50,  41,  32,  23,  88,  79,  78,
    67, 140, 137, 138, 141, 132, 121, 36, 42, 123, 51,  117, 126, 118, 131,
};

int main(int argc, char *argv[]) try {
  X50Q dev;
  // We start with built-in 6 (the profile activated with fn+numpad 0)
  auto status = dev.status();
  dev.set_builtin(6);
  // Initialize a buffer for color values. At the beginning, everything is black (aka RGB 0, 0, 0)
  std::uint8_t buf[3][144] = {};
  // Iterate over all keys, can change to mapping for UK layout
  for (auto i : mapping) {
    // Set the key at index i to white by settings all three components to maximum.
    buf[0][i] = buf[1][i] = buf[2][i] = 0xFF;
    // Now activate the color
    dev.apply_colors_idle(buf);
    // Assign the same color after pressing the key
    dev.apply_colors_active(buf);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(200ms);
    // Reset to black such that only one key is white at a time.
    buf[0][i] = buf[1][i] = buf[2][i] = 0x0;
  }
  // Return to the previously active builtin profile (Of course, customizations get lost in the process)
  fmt::print("Resetting to {}\n", status.profile);
  dev.set_builtin(status.profile);
  return 0;
} catch (const char *str) {
  fmt::print(stderr, "Error: {}\n", str);
  return -1;
} catch (const std::exception &ex) {
  fmt::print(stderr, "Error: {}\n", ex.what());
  return -1;
}
