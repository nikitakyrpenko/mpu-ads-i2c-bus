#include "IDevice.hpp"

#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

bool IDevice::select(int fd)
{
  if (fd <= 0) {
    throw std::runtime_error("File descriptor is not initialized");
  }
  return ioctl(fd, I2C_SLAVE, address()) >= 0;
}

ssize_t IDevice::write(const uint8_t* buf, size_t len, int fd)
{
  size_t bytes_write = 0;
  while (bytes_write < len) {
    ssize_t n = ::write(fd, buf + bytes_write, len - bytes_write);
    if (n < 0) {
      return n;
    }
    bytes_write += static_cast<size_t>(n);
  }
  return static_cast<ssize_t>(bytes_write);
}

ssize_t IDevice::read(uint8_t* buf, size_t len, int fd)
{
  size_t bytes_read = 0;
  while (bytes_read < len) {
    ssize_t n = ::read(fd, buf + bytes_read, len - bytes_read);
    if (n <= 0) {
      return n;
    }
    bytes_read += n;
  }
  return static_cast<ssize_t>(bytes_read);
}

bool IDevice::setup(int fd)
{
  (void)fd;
  return true;
}

IDevice::~IDevice() {}