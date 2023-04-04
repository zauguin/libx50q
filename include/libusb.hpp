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

#include <cassert>
#include <chrono>
#include <libusb.h>
#include <memory>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

namespace mfk::libusb {
inline class libusb_error_category : public std::error_category {
 public:
  const char *name() const noexcept override { return "libusb"; }
  std::string message(int code) const noexcept override { return libusb_error_name(code); }
} libusb_error_category;

struct ConfigDescriptionDeleter {
  void operator()(libusb_config_descriptor *desc) const noexcept {
    libusb_free_config_descriptor(desc);
  }
};

class Device {
 public:
  class Handle {
    struct Deleter {
      void operator()(libusb_device_handle *ptr) noexcept { libusb_close(ptr); }
    };
    std::unique_ptr<libusb_device_handle, Deleter> handle;

   public:
    Handle(const Device &device) {
      libusb_device_handle *dev;
      if (int err = libusb_open(device, &dev)) throw std::system_error(err, libusb_error_category);
      handle.reset(dev);
    }
    operator libusb_device_handle *() const noexcept { return handle.get(); }
    void claim(int interface) {
      if (int err = libusb_claim_interface(handle.get(), interface))
        throw std::system_error(err, libusb_error_category);
    }
    std::span<const std::byte>
    send_interrupt(std::uint8_t endpoint, std::span<const std::byte> data,
                   std::chrono::duration<unsigned int, std::ratio<1, 1000>> timeout = {}) {
      assert((endpoint & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT);
      int transferred;
      if (auto err = libusb_interrupt_transfer(
              handle.get(), endpoint,
              reinterpret_cast<unsigned char *>(const_cast<std::byte *>(data.data())), data.size(),
              &transferred, timeout.count()))
        throw std::system_error(err, libusb_error_category);
      return data.subspan(transferred);
    }
  };

 private:
  libusb_device *dev = nullptr;

 public:
  Device() noexcept = default;
  explicit Device(libusb_device *dev) noexcept: dev(dev) {}
  Device(const Device &other) noexcept: dev(other ? libusb_ref_device(other.dev) : nullptr) {}
  Device(Device &&other) noexcept: dev(std::exchange(other.dev, nullptr)) {}
  Device &operator=(const Device &other) noexcept {
    if (&other == this) return *this;
    if (dev) libusb_unref_device(dev);
    dev = other ? libusb_ref_device(other.dev) : nullptr;
    return *this;
  }
  Device &operator=(Device &&other) noexcept {
    dev = std::exchange(other.dev, dev);
    return *this;
  }
  ~Device() noexcept {
    if (dev) libusb_unref_device(dev);
  }
  explicit operator bool() const noexcept { return dev; }
  operator libusb_device *() const noexcept { return dev; }

  libusb_device_descriptor device_descriptor() const noexcept {
    assert(dev);
    libusb_device_descriptor desc;
    (void)libusb_get_device_descriptor(dev, &desc); // Documented to always succeed
    return desc;
  }

  std::unique_ptr<libusb_config_descriptor, ConfigDescriptionDeleter>
  active_config_descriptor() const {
    assert(dev);
    libusb_config_descriptor *desc;
    if (int err = libusb_get_active_config_descriptor(dev, &desc))
      throw std::system_error(err, libusb_error_category);
    return std::unique_ptr<libusb_config_descriptor, ConfigDescriptionDeleter>(desc);
  }

  Handle open() { return Handle(*this); }
};
class Context {
  struct Deleter {
    void operator()(libusb_context *ptr) noexcept { libusb_exit(ptr); }
  };
  std::unique_ptr<libusb_context, Deleter> handle;

 public:
  Context() {
    libusb_context *context;
    if (int err = libusb_init(&context)) throw std::system_error(err, libusb_error_category);
    handle.reset(context);
  }
  operator libusb_context *() const noexcept { return handle.get(); }
  std::vector<Device> list() const {
    libusb_device **list;
    auto count = libusb_get_device_list(handle.get(), &list);
    if (count < 0) throw std::system_error(count, libusb_error_category);
    std::vector<Device> devices;
    try {
      devices.reserve(count);
    } catch (...) { libusb_free_device_list(list, true); }
    for (auto *device : std::span(list, count))
      devices.emplace_back(device);
    libusb_free_device_list(list, false);
    return devices;
  }
  static Context &get() {
    static Context instance;
    return instance;
  }
};
} // namespace mfk::libusb
