#ifndef X50Q_PROFILE_HPP
#define X50Q_PROFILE_HPP
#include "x50q.hpp"

#include <array>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <system_error>
#include <type_traits>

struct Profile {
  std::uint32_t version = 0x00010000;
  mfk::ByteSeconds active_duration[144];
  mfk::X50Q::Effect effects_active[144];
  std::uint8_t colors_active[3][144];
  mfk::X50Q::Effect effects_idle[144];
  std::uint8_t colors_idle[3][144];
  void apply(mfk::X50Q &x50q) {
    x50q.apply_active_duration(active_duration);
    x50q.apply_effects_active(effects_active);
    x50q.apply_colors_active(colors_active);
    x50q.apply_effects_idle(effects_idle);
    x50q.apply_colors_idle(colors_idle);
  }
};
static_assert(std::endian::native == std::endian::little && sizeof(Profile) == 4 + 144 * 9);

inline std::istream &operator>>(std::istream &stream, Profile &profile) {
  return stream.read(reinterpret_cast<char *>(&profile), sizeof(Profile));
}

inline std::ostream &operator<<(std::ostream &stream, const Profile &profile) {
  return stream.write(reinterpret_cast<const char *>(&profile), sizeof(Profile));
}
#endif
