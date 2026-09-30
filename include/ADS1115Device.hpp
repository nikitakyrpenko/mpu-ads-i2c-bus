#pragma once

#include "IDevice.hpp"

#include <chrono>
#include <cstdint>

class ADS1115Device : public IDevice {
  uint16_t address() override;
  const char* device() override;
  std::chrono::milliseconds poll_interval() override;

public:
  bool is_alive(int fd) override;
  bool setup(int fd) override;
  float to_voltage(uint8_t* buf);
  void poll(int fd) override;
};
