#pragma once

#include <unistd.h>
#include <chrono>
#include <cstddef>
#include <cstdint>

class IDevice {
public:
  virtual uint16_t address() = 0;
  virtual bool is_alive(int fd) = 0;
  virtual void poll(int fd) = 0;  // read + print
  virtual const char* device() = 0;
  virtual std::chrono::milliseconds poll_interval() = 0;
  virtual ~IDevice() = 0;

  bool select(int fd);  // ioctl (shift controll to this device)
  virtual bool setup(int fd);

protected:
  ssize_t read(uint8_t* buf, size_t len, int fd);         // read
  ssize_t write(const uint8_t* buf, size_t len, int fd);  // write
};