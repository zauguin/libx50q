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

#ifndef X50Q_HPP
#define X50Q_HPP
#include "hidapi.hpp"
#include "libusb.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <exception>
#include <fmt/format.h>
#include <optional>
#include <span>
#include <thread>
#include <vector>

namespace mfk {
using namespace hidapi;

// These exceptions are always bugs. The only reason we have an exception here at all
// is that due to the current state of development, this kind of bug is very likely to happen
// and you should report them.
class ProtocolException : public std::exception {
  std::uint16_t code_;
  std::string message;
  std::vector<std::byte> data;

 public:
  ProtocolException(std::uint16_t code, std::span<const std::byte> context): code_(code) {
    std::ranges::copy(context, std::back_inserter(data));
    message = fmt::format("Error while communicating with the keyboard. Please create an issue at "
                          "https://github.com/zauguin/libx50q/issues describing what you did when "
                          "you got this message and including the following information: \n\n"
                          "Error code: {}\nContext: ",
                          code);
    for (auto b : data)
      message += fmt::format("{:02x}", std::uint8_t(b));
  }
  auto code() { return code_; }
  const char *what() const noexcept override { return message.c_str(); }
  void complain() {
    fmt::print("FATAL ERROR: {}\n\n", message);
    std::terminate();
  }
};

// For durations, we need a 8 bit seconds type:
using ByteSeconds = std::chrono::duration<std::uint8_t>;
// We need this to have "the obvious" binary representation.
// That's not guaranteed by the standard, but in practice it's a reasonable assumption.
// If the library does something crazy, we can probably catch that with a simple check:
static_assert(1 == sizeof(ByteSeconds), "Your standard library implementation is not supported");

class X50Q {
 public:
  enum class Effect : std::uint8_t {
    SetColor      = 0,
    Breadth       = 1,
    Cycle         = 2,
    Blink         = 4,
    Ripple        = 5,
    InwardsRipple = 6,
    Laser         = 7
  };

  struct Status {
    std::uint8_t profile;
    std::uint8_t unknown_1;
    std::uint8_t firmware_version; // Mostly a guess, but currently it fits
    std::uint8_t unknown_2;
    std::uint8_t unknown_3;
  };

 private:
  libusb::Device::Handle output;
  std::uint8_t endpoint;
  hidapi::HidDevice input;
  std::function<void(std::uint8_t)> profile_change_callback /*= [](std::uint8_t profile) {
    fmt::print("Changed profile to {}.\n", profile);
  }*/;
  std::function<void(bool)> volume_key_callback;

  std::array<std::byte, 7> generic_exchange(std::span<const std::byte, 64> buffer) {
    auto remaining = output.send_interrupt(endpoint, buffer);
    if (!remaining.empty())
      throw ProtocolException(0, remaining);
    while (true) {
      // We allocate 10 bytes even though we only expect 9 bytes. This allows us to detect if too
      // much data was provided.
      std::array<std::byte, 10> response;
      auto length = input.read(response).size();
      if (length == 0) continue;
      if (response[0] != std::byte(8))
        continue; // This happens e.g. when multimedia keys are pressed or the volume is changed
      if (length != 9) throw ProtocolException(1, response);
      switch (response[1]) {
      case std::byte(2): {
        constexpr std::array<std::uint8_t, 7> notification_structure = {0x03, 0x24, 0xf0, 0x20,
                                                                        0x2b, 0x00, 0x00};
        for (int i = 0; i != notification_structure.size(); ++i) {
          if (i != 5 && std::byte(notification_structure[i]) != response[i + 2])
            throw ProtocolException(2, response);
        }
        auto profile = std::uint8_t(response[7]);
        if (profile == 0 || profile > 6) throw ProtocolException(2, response);
        if (profile_change_callback) profile_change_callback(profile);
      } break;
      case std::byte(0x67): {
        constexpr std::array<std::uint8_t, 7> notification_structure = {0x0c, 0x07, 0x73, 0x00,
                                                                        0x00, 0x00, 0x00};
        for (int i = 0; i != notification_structure.size(); ++i) {
          if (i != 3 && std::byte(notification_structure[i]) != response[i + 2]) {
            throw ProtocolException(3, response);
          }
        }
        if (volume_key_callback) switch (response[5]) {
          case std::byte(0): volume_key_callback(false); break;
          case std::byte(1): volume_key_callback(true); break;
          default: throw ProtocolException(3, response);
          }
      } break;
      case std::byte(0):
        std::array<std::byte, 7> data;
        std::copy(response.begin() + 2, response.end(), data.begin());
        return data;
      default: throw ProtocolException(4, response);
      }
    }
  }

  auto exchange_block(std::byte cmd, std::byte sub_cmd, std::uint8_t index = {},
                      std::span<const std::byte> payload = {}) {
    assert(payload.size() <= 60);
    std::array<std::byte, 64> msg = {std::byte(7), cmd, sub_cmd, std::byte(index)};
    std::ranges::copy(payload, msg.begin() + 4);
    auto response = generic_exchange(msg);
    if (std::byte(0x8) != response[0]) throw ProtocolException(5, response);
    if (cmd != response[1]) throw ProtocolException(6, response);
    if (std::ranges::any_of(std::span(response).subspan(2),
                            [](std::byte b) { return b != std::byte(); }))
      throw ProtocolException(7, response);
  }

  auto exchange(std::byte cmd, std::byte sub_cmd, std::uint16_t max_data_size = 0,
                std::span<const std::byte> payload = {}) {
    assert(payload.size() <= max_data_size);
    assert(max_data_size <= 60 * 256);
    const int blocks = max_data_size ? (max_data_size + 59) / 60 : 1;
    for (std::uint8_t i = 0; blocks != i; ++i) {
      auto this_payload = payload.size() <= 60 ? payload : payload.subspan(0, 60);
      payload           = payload.subspan(this_payload.size());
      exchange_block(cmd, sub_cmd, i, this_payload);
    }
  }

  void setup();
  void set_builtin_(std::uint8_t index) { exchange(std::byte(0x01), std::byte(index)); }

  static X50Q find_device(std::uint16_t vid, std::uint16_t pid) {
    auto &hid = hidapi::HidApi::get();
    std::optional<std::string> path;
    for (auto &&dev : hid.enumerate(vid, pid)) {
      if (1 == dev.interface_number) {
        path = dev.path;
        break;
      }
    }
    if (!path) throw std::runtime_error("X50Q keyboard not detected");
    auto in_handle = hid.open(path->c_str());

    auto &context = libusb::Context::get();
    libusb::Device out_handle;
    for (auto &&dev : context.list()) {
      auto desc = dev.device_descriptor();
      if (desc.idVendor == vid && desc.idProduct == pid) { out_handle = std::move(dev); }
    }
    if (!out_handle) throw std::runtime_error("Unable to find keyboard with libusb");

    return X50Q(std::move(in_handle), std::move(out_handle));
  }

  // Find the output interrup endpoint id for configuration 0, interface 2, endpoint 0
  static std::uint16_t find_endpoint(const libusb::Device &dev) {
    auto config = dev.active_config_descriptor();
    assert(config->bNumInterfaces == 3);
    auto out_iface = config->interface[2];
    assert(out_iface.num_altsetting == 1);
    auto alternate = out_iface.altsetting[0];
    assert(alternate.bNumEndpoints == 1);
    auto endpoint = alternate.endpoint[0];
    assert(endpoint.bmAttributes == LIBUSB_ENDPOINT_TRANSFER_TYPE_INTERRUPT);
    assert((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT);
    assert(endpoint.wMaxPacketSize == 64);
    return endpoint.bEndpointAddress;
  }

 public:
  X50Q(hidapi::HidDevice input, libusb::Device output):
      output(output.open()), endpoint(find_endpoint(output)), input(std::move(input)) {}
  X50Q(std::uint16_t vid = 0x24f0, std::uint16_t pid = 0x202b): X50Q(find_device(vid, pid)) {}

  Status status() {
    std::array<std::byte, 64> msg = {std::byte(7), std::byte(0x81)};
    auto response                 = generic_exchange(msg);
    if (std::byte(0x8) != response[0]) throw ProtocolException(8, response);
    if (std::byte(0x81) != response[1]) throw ProtocolException(9, response);

    Status status;
    static_assert(sizeof status + 2 == response.size());
    memcpy(&status, response.data() + 2, sizeof status);
    return status;
  }

  /**
   * For index between 1 and 6, activate the corresponding builtin profile.
   * Can also be called with index == 0 which does something slow.
   */
  void set_builtin(std::uint8_t index) {
    assert(index > 0 && index <= 6);
    return set_builtin_(index);
  }

  /** Change the idle color
   *
   * Yes, a transposed matrix would be much nicer.
   *
   * The order is not obvious and best determined by trial and error.
   * (Or looking at sample code)
   */
  void apply_colors_idle(std::span<const std::uint8_t[144]> data = {}) {
    exchange(std::byte(0x09), std::byte(0x06), 3 * 144, as_bytes(data));
  }

  /** Change the color used after the key is pressed.
   *
   * First 144 red component values are read, then 144 green, finally 144 blue components.
   */
  void apply_colors_active(std::span<const std::uint8_t[144]> data = {}) {
    exchange(std::byte(0x0a), std::byte(0x06), 3 * 144, as_bytes(data));
  }

  /** Change the effects used after the key is pressed. Upto 144 effects can be provided. */
  void apply_effects_idle(std::span<const Effect> data = {}) {
    exchange(std::byte(0x0d), std::byte(0x06), 144, as_bytes(data));
  }

  /** Change the effects used after the key is pressed. Upto 144 effects can be provided. */
  void apply_effects_active(std::span<const Effect> data = {}) {
    exchange(std::byte(0x0e), std::byte(0x06), 144, as_bytes(data));
  }

  /** Change the effects used after the key is pressed. Upto 144 effects can be provided. */
  void apply_active_duration(std::span<const std::chrono::duration<std::uint8_t>> data = {}) {
    exchange(std::byte(0x0f), std::byte(0x06), 144, as_bytes(data));
  }
};

// Not sure if this is useful for anything. It replicates what the Windows program does when the
// keyboard gets connected. It seems to be some kind of reset procedure, but since the keyboard
// resets after being disconnected anyway I'm not sure why it would be needed. Also it's slow...
inline void X50Q::setup() {
  auto state = status();
  if (state.firmware_version != 0x40)
    fmt::print(stderr, "This library has not been tested with your firmware version.\n");

  set_builtin(6);

  apply_effects_active();
  apply_effects_idle();

  // The previous ones are very fast, here we have more than 4 seconds delay for each of the two.
  // The second ones set the profile to 06, not sure what 00 does here. Maybe some kind of device
  // reset?
  set_builtin_(0);
  set_builtin(6);
}
} // namespace mfk
#endif
