#include "profile.hpp"

#include <array>
#include <fstream>
#include <iostream>

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
    if ((profile.version & 0xFFFF0000U) != 0x00010000) {
      fmt::print(stderr, "Unsupported profile version");
      return 2;
    }
  }
  mfk::X50Q x50q;
  profile.apply(x50q);
  return 0;
}
