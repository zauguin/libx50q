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

#ifndef HIDAPI_HPP
#define HIDAPI_HPP
#include <codecvt>
#include <fmt/format.h>
#include <hidapi.h>
#include <locale>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>

namespace mfk::hidapi {
class HidError : public std::runtime_error {
  std::wstring str;
  hid_device *dev;

  static std::string wchar_to_utf8(const wchar_t *str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> cv;
    return cv.to_bytes(str);
  }

 public:
  explicit HidError(hid_device *dev = nullptr):
      runtime_error(wchar_to_utf8(hid_error(dev))), dev(dev) {}
};

class HidApi;

class HidEnumeration {
  struct Deleter {
    void operator()(hid_device_info *ptr) { hid_free_enumeration(ptr); }
  };
  std::unique_ptr<hid_device_info, Deleter> ptr;

 public:
  class iterator {
    hid_device_info *ptr;

   public:
    iterator(hid_device_info *ptr = nullptr): ptr(ptr) {}
    auto operator<=>(iterator other) const { return ptr <=> other.ptr; }
    bool operator==(const iterator &other) const = default;
    bool operator!=(const iterator &other) const = default;
    iterator &operator++() {
      ptr = ptr->next;
      return *this;
    }
    hid_device_info &operator*() { return *ptr; }
    hid_device_info *operator->() { return ptr; }
  };
  explicit HidEnumeration(hid_device_info *info): ptr(info) {}
  iterator begin() { return iterator(ptr.get()); }
  iterator end() { return iterator(); }
};

class HidDevice {
  static constexpr std::size_t buffer_length = 255;
  struct Deleter {
    void operator()(hid_device *ptr) { hid_close(ptr); }
  };
  std::unique_ptr<hid_device, Deleter> ptr;

 public:
  explicit HidDevice(hid_device *dev): ptr(dev) {}
  hid_device *native_handle() { return ptr.get(); }
  std::wstring manufacturer() {
    wchar_t buffer[buffer_length];
    if (hid_get_manufacturer_string(native_handle(), buffer, buffer_length))
      throw HidError(native_handle());
    return buffer;
  }
  std::wstring product() {
    wchar_t buffer[buffer_length];
    if (hid_get_product_string(native_handle(), buffer, buffer_length))
      throw HidError(native_handle());
    return buffer;
  }
  std::wstring serial_number() {
    wchar_t buffer[buffer_length];
    if (hid_get_serial_number_string(native_handle(), buffer, buffer_length))
      throw HidError(native_handle());
    return buffer;
  }
  std::wstring indexed_string(int index) {
    wchar_t buffer[buffer_length];
    if (hid_get_indexed_string(native_handle(), index, buffer, buffer_length))
      throw HidError(native_handle());
    return buffer;
  }
  std::size_t write(std::span<const std::byte> data) {
    auto written = hid_write(native_handle(), reinterpret_cast<const unsigned char *>(data.data()),
                             data.size());
    if (written == -1) throw HidError(native_handle());
    return written;
  }
  std::span<std::byte> read(std::span<std::byte> buffer) {
    auto read =
        hid_read(native_handle(), reinterpret_cast<unsigned char *>(buffer.data()), buffer.size());
    if (read == -1) throw HidError(native_handle());
    return buffer.subspan(0, read);
  }
};

class HidApi {
 private:
  HidApi() {
    if (hid_init()) {
      fmt::print(stderr, "Failed to initialize hidapi");
      std::terminate();
    }
  }

 public:
  HidApi(const HidApi &) = delete;
  HidApi &operator=(const HidApi &) = delete;
  ~HidApi() {
    if (hid_exit()) {
      fmt::print(stderr, "Failed to finalize hidapi");
      std::terminate();
    }
  }
  const hid_api_version &version() { return *hid_version(); }
  const char *version_string() { return hid_version_str(); }
  static HidApi &get() {
    static HidApi instance;
    return instance;
  }
  HidDevice open(std::uint16_t vid, std::uint16_t pid) {
    auto handle = hid_open(vid, pid, nullptr);
    if (!handle) throw HidError();
    return HidDevice(handle);
  }
  HidDevice open(const char *path) {
    auto handle = hid_open_path(path);
    if (!handle) throw HidError();
    return HidDevice(handle);
  }
  HidEnumeration enumerate(std::uint16_t vid = 0, std::uint16_t pid = 0) {
    return HidEnumeration(hid_enumerate(vid, pid));
  }
};
} // namespace mfk::hidapi
#endif
