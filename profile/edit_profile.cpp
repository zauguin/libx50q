#include "profile.hpp"
#include "x50q.hpp"

#include <array>
#include <charconv>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <unordered_map>

const std::unordered_map<std::string, std::uint8_t> key_mapping = {
    {"'", 66},
    {",", 85},
    {"-", 72},
    {".", 86},
    {"/", 76},
    {"0", 81},
    {"1", 37},
    {"2", 28},
    {"3", 19},
    {"4", 10},
    {"5", 15},
    {"6", 1},
    {"7", 6},
    {"8", 91},
    {"9", 96},
    {";", 74},
    {"=", 64},
    {"[", 65},
    {"\\", 56},
    {"]", 69},
    {"`", 46},
    {"a", 39},
    {"b", 13},
    {"backspace", 60},
    {"c", 24},
    {"caps_lock", 48},
    {"d", 21},
    {"del", 101},
    {"down", 137},
    {"e", 20},
    {"end", 103},
    {"enter", 58},
    {"esc", 45},
    {"f", 25},
    {"f1", 27},
    {"f10", 63},
    {"f11", 68},
    {"f12", 54},
    {"f2", 18},
    {"f3", 9},
    {"f4", 14},
    {"f5", 0},
    {"f6", 5},
    {"f7", 90},
    {"f8", 95},
    {"f9", 77},
    {"fn", 79},
    {"forward", 129},
    {"g", 12},
    {"h", 3},
    {"home", 105},
    {"i", 82},
    {"ins", 61},
    {"j", 93},
    {"k", 83},
    {"l", 84},
    {"left", 140},
    {"left_alt", 32},
    {"left_ctrl", 50},
    {"left_light_bottom", 42},
    {"left_light_top", 36},
    {"left_shift", 49},
    {"light", 111},
    {"m", 94},
    {"menu", 78},
    {"meta", 41},
    {"n", 4},
    {"num", 102},
    {"num_0", 141},
    {"num_1", 136},
    {"num_2", 109},
    {"num_3", 127},
    {"num_4", 139},
    {"num_5", 112},
    {"num_6", 130},
    {"num_7", 142},
    {"num_8", 115},
    {"num_9", 133},
    {"num_divide", 110},
    {"num_dot", 132},
    {"num_enter", 121},
    {"num_minus", 119},
    {"num_mult", 128},
    {"num_plus", 124},
    {"o", 87},
    {"p", 73},
    {"page_down", 100},
    {"page_up", 106},
    {"pause", 104},
    {"play", 120},
    {"print", 59},
    {"q", 38},
    {"r", 11},
    {"right", 138},
    {"right_alt", 88},
    {"right_ctrl", 67},
    {"right_light_bottom", 51},
    {"right_light_top", 123},
    {"right_shift", 70},
    {"s", 30},
    {"scrlk", 99},
    {"space", 23},
    {"t", 16},
    {"tab", 47},
    {"u", 92},
    {"up", 135},
    {"v", 22},
    {"volume_bottom_left", 131},
    {"volume_bottom_right", 118},
    {"volume_top_left", 117},
    {"volume_top_right", 126},
    {"w", 29},
    {"x", 33},
    {"y", 2},
    {"z", 31},
};

std::istream &operator>>(std::istream &stream, mfk::X50Q::Effect &effect) {
  std::string name;
  stream >> name;
  if (name == "set-color")
    effect = mfk::X50Q::Effect::SetColor;
  else if (name == "breadth")
    effect = mfk::X50Q::Effect::Breadth;
  else if (name == "blink")
    effect = mfk::X50Q::Effect::Blink;
  else if (name == "cycle")
    effect = mfk::X50Q::Effect::Cycle;
  else if (name == "inwards-ripple")
    effect = mfk::X50Q::Effect::InwardsRipple;
  else if (name == "ripple")
    effect = mfk::X50Q::Effect::Ripple;
  else if (name == "laser")
    effect = mfk::X50Q::Effect::Laser;
  else {
    fmt::print(stderr, "Unknown effect. The supported effects are 'set-color', 'breadth', 'blink', "
                       "'cycle', 'inwards-ripple', 'ripple' and 'laser'.\n");
    std::exit(10);
  }
  return stream;
}

void read_rgb(std::span<std::uint8_t[144], 3> buffer, std::uint8_t index, std::istream &stream) {
  std::string color_str;
  stream >> color_str;
  std::uint8_t r, g, b;
  auto color = color_str.c_str();
  if (color_str.size() == 3) {
    auto result = std::from_chars(color, color + 1, r, 16);
    if (result.ptr != color + 1) {
      fmt::print(stderr, "Unable to parse red component\n");
      std::exit(11);
    }
    r *= 17;
    result = std::from_chars(color + 1, color + 2, g, 16);
    if (result.ptr != color + 2) {
      fmt::print(stderr, "Unable to parse green component\n");
      std::exit(11);
    }
    g *= 17;
    result = std::from_chars(color + 2, color + 3, b, 16);
    if (result.ptr != color + 3) {
      fmt::print(stderr, "Unable to parse green component\n");
      std::exit(11);
    }
    b *= 17;
  } else if (color_str.size() == 6) {
    auto result = std::from_chars(color, color + 2, r, 16);
    if (result.ptr != color + 2) {
      fmt::print(stderr, "Unable to parse red component\n");
      std::exit(11);
    }
    result = std::from_chars(color + 2, color + 4, g, 16);
    if (result.ptr != color + 4) {
      fmt::print(stderr, "Unable to parse green component\n");
      std::exit(11);
    }
    result = std::from_chars(color + 4, color + 6, b, 16);
    if (result.ptr != color + 6) {
      fmt::print(stderr, "Unable to parse green component\n");
      std::exit(11);
    }
  } else {
    fmt::print(stderr, "Unable to parse color\n");
    std::exit(11);
  }
  buffer[0][index] = r;
  buffer[1][index] = g;
  buffer[2][index] = b;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fmt::print("Usage: {} <file name>\n", argv[0]);
    return -1;
  }
  Profile profile;
  {
    std::ifstream file(argv[1]);
    if (!(file >> profile)) {
      fmt::print(stderr, "Unable to read profile");
      return 1;
    }
    if ((profile.version & 0xFFFFFF00U) != 0x00010000) {
      fmt::print(stderr, "Unsupported profile version");
      return 2;
    }
  }
  profile.version  = 0x00010000;
  std::uint8_t key = 255;
  std::string operation;
  while (std::cin >> operation) {
    if (operation == "apply") {
      mfk::X50Q x50q;
      profile.apply(x50q);
    } else if (operation == "save") {
      std::ofstream file(argv[1]);
      if (!(file << profile)) {
        fmt::print(stderr, "Unable to write profile");
        return 1;
      }
    } else if (operation == "quit") {
      std::ofstream file(argv[1]);
      if (!(file << profile)) {
        fmt::print(stderr, "Unable to write profile");
        return 1;
      }
    } else if (operation == "key") {
      unsigned int num;
      std::cin >> num;
      if (num >= 144) {
        fmt::print(stderr, "Invalid keycode provided\n");
        return 3;
      } else
        key = num;
    } else if (operation == "name") {
      std::string name;
      std::cin >> name;
      auto iter = key_mapping.find(name);
      if (key_mapping.end() == iter) {
        fmt::print(stderr, "Unknown keyname\n");
        return 3;
      } else
        key = iter->second;
    } else if (key == 255) {
      fmt::print(stderr, "No other commands are allowed until a key has been selected\n");
      return 4;
    } else if (operation == "active-duration") {
      unsigned int seconds;
      std::cin >> seconds;
      if (seconds >= 0x100) {
        fmt::print(stderr, "Duration must be less than 256 seconds\n");
        return 6;
      }
      profile.active_duration[key] = mfk::ByteSeconds(seconds);
    } else if (operation == "active-effect") {
      mfk::X50Q::Effect effect;
      std::cin >> effect;
      profile.effects_active[key] = effect;
    } else if (operation == "idle-effect") {
      mfk::X50Q::Effect effect;
      std::cin >> effect;
      profile.effects_idle[key] = effect;
    } else if (operation == "active-effect") {
      mfk::X50Q::Effect effect;
      std::cin >> effect;
      profile.effects_active[key] = effect;
    } else if (operation == "idle-color") {
      read_rgb(profile.colors_idle, key, std::cin);
    } else if (operation == "active-color") {
      read_rgb(profile.colors_active, key, std::cin);
    } else {
      fmt::print(stderr, "Unknown command. The supported commands are 'key', 'name', 'apply', "
                         "'save', 'quit', 'active-duration'. "
                         "'active-effect', 'active-color', 'idle-effect' and 'idle-color'");
      return 5;
    }
  }

  return 0;
}
