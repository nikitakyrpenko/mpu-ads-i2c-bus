
#include "IDevice.hpp"
#include "MPU6050Device.hpp"
#include "ADS1115Device.hpp"

#include <memory>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <span>
#include <string_view>
#include <thread>
#include <vector>

uint16_t hex_to_int16_t(std::string_view hex)
{
  if (hex.starts_with("0x") || hex.starts_with("0X")) {
    hex.remove_prefix(2);
  }

  unsigned int raw_val = 0;
  auto [ptr, ec] = std::from_chars(hex.data(), hex.data() + hex.size(), raw_val, 16);
  if (ec != std::errc()) {
    throw std::invalid_argument("bad hex address");
  }
  if (raw_val > std::numeric_limits<uint16_t>::max()) {
    throw std::out_of_range("Value exceeds uint16_t capacity");
  }
  return static_cast<uint16_t>(raw_val);
}

int open_fd(const char* ser)
{
  int fd = open(ser, O_RDWR);
  if (fd < 0) {
    std::cerr << "Failed to open the I2C bus.\n";
    return fd;
  }
  return fd;
}

int main(int argc, const char** argv)
{
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <i2c-bus-path> [address ...]\n";
    return 1;
  }
  std::span<const char*> args(argv + 2, argc - 2);

  std::vector<std::unique_ptr<IDevice>> devices;
  devices.reserve(2);

  devices.emplace_back(std::make_unique<MPU6050Device>());
  devices.emplace_back(std::make_unique<ADS1115Device>());

  std::erase_if(devices, [&](const std::unique_ptr<IDevice>& device) {
    return std::none_of(args.begin(), args.end(), [&](const char* addr) { return hex_to_int16_t(addr) == device->address(); });
  });

  if (devices.empty()) {
    std::cerr << "No known devices match the given addresses.\n";
    return 1;
  }

  int fd = open_fd(argv[1]);

  std::vector<IDevice*> alive_devices;
  for (auto& d : devices) {
    if (d->is_alive(fd)) {
      d->setup(fd);
      alive_devices.push_back(d.get());
    }
    else {
      std::cerr << "Device : " << d->device() << " is not responding, skipping.\n";
    }
  }

  std::vector<std::chrono::steady_clock::time_point> next_poll(alive_devices.size(), std::chrono::steady_clock::now());

  while (true) {
    auto now = std::chrono::steady_clock::now();
    for (size_t i = 0; i < alive_devices.size(); ++i) {
      if (now >= next_poll[i]) {
        alive_devices[i]->poll(fd);
        next_poll[i] = now + alive_devices[i]->poll_interval();
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}