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
#include <iostream>
#include <cstdint>
#include <exception>
#include <fmt/format.h>
#include <optional>
#include <span>
#include <thread>
#include <vector>

using namespace mfk;

std::array<std::uint8_t, 117> ukMapping = {
    45, 27,  18,  9,   14,  0,   5,   90, 95, 77,  63,  68,  54,  59,  99,  104, 111, 120, 129, 46,
    37, 28,  19,  10,  15,  1,   6,   91, 96, 81,  72,  64,  60,  61,  105, 106, 102, 110, 128, 119,
    47, 38,  29,  20,  11,  16,  2,   92, 82, 87,  73,  65,  69,  56,  101, 103, 100, 142, 115, 133,
    48, 39,  30,  21,  25,  12,  3,   93, 83, 84,  74,  66,  57, 58,  139, 112, 130, 124, 49,  40, 31, 33,
    24, 22,  13,  4,   94,  85,  86,  76, 70, 135, 136, 109, 127, 50,  41,  32,  23,  88,  79,  78,
    67, 140, 137, 138, 141, 132, 121, 36, 42, 123, 51,  117, 126, 118, 131,
};

struct ColorData {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
};

// For a UK layout rainbow effect
std::vector<ColorData> colors = {
    {0, 82, 255}, {0, 82, 255}, {0, 136, 255}, {0, 255, 197}, {0, 255, 113}, {0, 255, 113}, {179, 255, 0}, {179, 255, 0}, {255, 200, 0}, {255, 85, 0}, {255, 85, 0}, {255, 62, 0}, {255, 62, 0}, {255, 46, 0}, {255, 0, 62}, {241, 0, 255}, {241, 0, 255}, {241, 0, 255}, {179, 0, 255}, // Top row 
    {0, 82, 255}, {0, 82, 255}, {0, 136, 255}, {0, 136, 255}, {0, 255, 197}, {0, 255, 197}, {0, 255, 113}, {0, 255, 113}, {179, 255, 0}, {179, 255, 0}, {255, 200, 0}, {255, 85, 0}, {255, 85, 0}, {255, 62, 0},  {255, 46, 0}, {255, 0, 62}, {241, 0, 255}, {241, 0, 255}, {118, 0, 255}, {72, 0, 255}, {72, 0, 255}, // Second row
    {0, 82, 255}, {0, 82, 255}, {0, 136, 255}, {0, 136, 255}, {0, 255, 197}, {0, 255, 197}, {0, 255, 113}, {0, 255, 113}, {179, 255, 0}, {179, 255, 0}, {255, 200, 0}, {255, 85, 0}, {255, 85, 0}, {255, 255, 255}, {255, 46, 0}, {255, 0, 62}, {255, 0, 62}, {241, 0, 255}, {241, 0, 255},  {72, 0, 255}, // Third row
    {0, 82, 255}, {0, 82, 255}, {0, 136, 255}, {0, 136, 255}, {0, 255, 197}, {0, 255, 197}, {0, 255, 113}, {0, 255, 113}, {179, 255, 0}, {179, 255, 0}, {255, 200, 0}, {255, 85, 0}, {255, 62, 0}, {255, 85, 0}, {241, 0, 255}, {241, 0, 255}, {179, 0, 255}, {72, 0, 255}, // Forth row + Enter and numpad +
    {0, 82, 255}, {0, 82, 255}, {0, 82, 255}, {0, 136, 255}, {0, 136, 255}, {0, 255, 197}, {0, 255, 197}, {0, 255, 113}, {0, 255, 113}, {179, 255, 0}, {179, 255, 0}, {255, 200, 0}, {255, 85, 0}, {255, 46, 0}, {255, 0, 62}, {241, 0, 255}, {179, 0, 255}, // Fith row
    {0, 82, 255}, {0, 82, 255}, {0, 82, 255}, {0, 255, 197}, {179, 255, 0}, {179, 255, 0}, {255, 200, 0}, {255, 85, 0}, {255, 46, 0}, {255, 46, 0}, {255, 0, 62}, {255, 0, 62}, {179, 0, 255}, {72, 0, 255}, // Sixth row + numpad return
    {0, 82, 255}, {0, 82, 255}, {179, 0, 255}, {179, 0, 255}, // set the side bar lights, first 2 are for the left side, second 2 for the right side.
    {210, 0, 255}, {210, 0, 255}, {210, 0, 255}, {210, 0, 255} // Set the volume wheel lights
};

int main(int argc, char *argv[]) try {
  X50Q dev;
  // We start with built-in 6 (the profile activated with fn+numpad 0)
  auto status = dev.status();
  dev.set_builtin(6);
  // Initialize a buffer for color values. At the beginning, everything is black (aka RGB 0, 0, 0)
  std::uint8_t buf[3][144] = {};

  using namespace std::chrono_literals;
  std::array<ByteSeconds, 144> durations;
  std::fill(durations.begin(), durations.end(), 1s);

  dev.apply_active_duration(durations);

  // Active effect
  std::array<X50Q::Effect, 144> effects;
  for (auto &e : effects)
    e = X50Q::Effect::Cycle;
  dev.apply_effects_active(effects);

  // Idle effect
  for (auto &e : effects)
    e = X50Q::Effect::SetColor;
  dev.apply_effects_idle(effects);

  int counter = 0;
  for (auto i : ukMapping) {
    // Set the key at index i to white by settings all three components to maximum.
    buf[0][i] = colors[counter].red;
    buf[1][i] = colors[counter].green;
    buf[2][i] = colors[counter].blue;
    counter++;
  }

  // Now activate the color
  dev.apply_colors_idle(buf);

  return 0;
} catch (const char *str) {
  fmt::print(stderr, "Error: {}\n", str);
  return -1;
} catch (const std::exception &ex) {
  fmt::print(stderr, "Error: {}\n", ex.what());
  return -1;
}
