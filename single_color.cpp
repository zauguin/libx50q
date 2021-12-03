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

#include "x50q.hpp"

#include <algorithm>
#include <charconv>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fmt::print("Usage: {} 00FF88\n", argv[0]);
    return 0;
  }
  std::uint8_t r, g, b;
  auto color  = argv[1];
  auto length = std::strlen(color);
  if (length == 3) {
    auto result = std::from_chars(color, color + 1, r, 16);
    if (result.ptr != color + 1) {
      fmt::print(stderr, "Unable to parse red component\n");
      return -1;
    }
    r *= 17;
    result = std::from_chars(color + 1, color + 2, g, 16);
    if (result.ptr != color + 2) {
      fmt::print(stderr, "Unable to parse green component\n");
      return -1;
    }
    g *= 17;
    result = std::from_chars(color + 2, color + 3, b, 16);
    if (result.ptr != color + 3) {
      fmt::print(stderr, "Unable to parse green component\n");
      return -1;
    }
    b *= 17;
  } else if (length == 6) {
    auto result = std::from_chars(color, color + 2, r, 16);
    if (result.ptr != color + 2) {
      fmt::print(stderr, "Unable to parse red component\n");
      return -1;
    }
    result = std::from_chars(color + 2, color + 4, g, 16);
    if (result.ptr != color + 4) {
      fmt::print(stderr, "Unable to parse green component\n");
      return -1;
    }
    result = std::from_chars(color + 4, color + 6, b, 16);
    if (result.ptr != color + 6) {
      fmt::print(stderr, "Unable to parse green component\n");
      return -1;
    }
  } else {
    fmt::print(stderr, "Unable to parse color\n");
    return -1;
  }

  std::uint8_t colors[3][144];
  std::ranges::fill(colors[0], r);
  std::ranges::fill(colors[1], g);
  std::ranges::fill(colors[2], b);
  mfk::X50Q dev;
  dev.apply_effects_active();
  dev.apply_colors_active(colors);
  dev.apply_effects_idle();
  dev.apply_colors_idle(colors);
}
