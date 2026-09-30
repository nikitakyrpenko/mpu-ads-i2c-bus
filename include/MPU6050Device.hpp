#pragma once

#include "IDevice.hpp"

#include <chrono>
#include <cstdint>
#include <ostream>

class MPU6050Device : public IDevice {
  uint16_t address() override;
  const char* device() override;
  std::chrono::milliseconds poll_interval() override;

public:
  struct Measure {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float temp;

    friend std::ostream& operator<<(std::ostream& os, const Measure& m);
  };

  bool is_alive(int fd) override;
  bool setup(int fd) override;
  Measure to_measure(uint8_t* buf);
  void poll(int fd) override;

  ~MPU6050Device() override;
};
