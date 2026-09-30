#include "MPU6050Device.hpp"

#include <cstddef>
#include <iostream>

namespace {
// CONFIG
static constexpr uint8_t FSYNC_OFF = 0b000;
static constexpr uint8_t DLPF_ON = 0b010;  // 1000Hz;

// SMPLRT_DIV
static constexpr uint8_t SMPLRT_DIV = 0;

// PWR_MGMT_1
static constexpr uint8_t DEVICE_RESET = 0;
static constexpr uint8_t SLEEP = 0;
static constexpr uint8_t CYCLE = 0;
static constexpr uint8_t TEMP_DIS = 0;
static constexpr uint8_t CLKSEL = 1;

static constexpr size_t MEASURE_SIZE = 14;  // 6 accel + 6 gyro + 2 temp

static constexpr float ACCEL_LSB_SENS = 16384.f;
static constexpr float GYRO_LSB_SENS = 131.f;
static constexpr float TEMP_LSB_SENS = 340.f;
static constexpr float TEMP_OFFSET_C = 36.53f;

// Internal sample rate (from SMPLRT_DIV) is far faster than useful for console output;
// poll_interval() uses a separate, human-readable console update rate instead.
static constexpr int MPU6050_CONSOLE_POLL_MS = 300;
}  // namespace

std::ostream& operator<<(std::ostream& os, const MPU6050Device::Measure& m)
{
  return os << "accel[g]: " << m.accel_x << ", " << m.accel_y << ", " << m.accel_z << " gyro[dps]: " << m.gyro_x << ", " << m.gyro_y << ", "
            << m.gyro_z << " temp[C]: " << m.temp << std::endl;
}

uint16_t MPU6050Device::address()
{
  return 0x68;
}
const char* MPU6050Device::device()
{
  return "MPU6050";
}
std::chrono::milliseconds MPU6050Device::poll_interval()
{
  return std::chrono::milliseconds(MPU6050_CONSOLE_POLL_MS);
}

bool MPU6050Device::is_alive(int fd)
{
  IDevice::select(fd);

  uint8_t reg = 0x75;
  uint8_t who_am_i;

  if (IDevice::write(&reg, 1, fd) < 1) {
    std::cerr << "Cannot write " << reg << "into device " << device();
    return false;
  }

  if (IDevice::read(&who_am_i, 1, fd) < 1) {
    std::cerr << "Cannot read " << reg << "into device " << device();
    return false;
  }

  if (who_am_i != 0x68) {
    std::cerr << "Device : " << device() << " WHO_AM_I mismatch, expected 0x68 got 0x" << std::hex << static_cast<int>(who_am_i) << std::dec
              << '\n';
    return false;
  }

  return true;
}

bool MPU6050Device::setup(int fd)
{
  // setup of FSYNC OFF + DLPF ON
  const uint8_t config_frame[2] = {0x1A, (FSYNC_OFF << 3) | DLPF_ON};

  if (IDevice::write(config_frame, sizeof(config_frame), fd) < 2) {
    std::cerr << "Cannot write " << config_frame[0] << " : " << config_frame[1] << "into device " << device();
    return false;
  }

  // setup SMPRT_DIV to obtain 1kHz frequancy
  const uint8_t SMPLRT_frame[2] = {0X19, SMPLRT_DIV};
  if (IDevice::write(SMPLRT_frame, sizeof(SMPLRT_frame), fd) < 2) {
    std::cerr << "Cannot write " << SMPLRT_frame[0] << " : " << SMPLRT_frame[1] << "into device " << device();
    return false;
  }

  // setup SLEEP 0 and CYCLE 0
  const uint8_t PWR_MGMT_frame[2] = {0X6B, (DEVICE_RESET << 7) | (SLEEP << 6) | (CYCLE << 5) | (TEMP_DIS << 3) | CLKSEL};
  if (IDevice::write(PWR_MGMT_frame, sizeof(PWR_MGMT_frame), fd) < 2) {
    std::cerr << "Cannot write " << PWR_MGMT_frame[0] << " : " << PWR_MGMT_frame[1] << "into device " << device();
    return false;
  }

  std::cout << "Device : " << device() << " | config applied : " << "FSYNC : off | " << "DLPF : on | " << " NO SLEEP | "
            << "Internal loop rate : 1kHz \n";
  return true;
}

MPU6050Device::Measure MPU6050Device::to_measure(uint8_t* buf)
{
  int16_t accel_x_raw = buf[0] << 8 | buf[1];
  int16_t accel_y_raw = buf[2] << 8 | buf[3];
  int16_t accel_z_raw = buf[4] << 8 | buf[5];

  int16_t temp_raw = buf[6] << 8 | buf[7];

  int16_t gyro_x_raw = buf[8] << 8 | buf[9];
  int16_t gyro_y_raw = buf[10] << 8 | buf[11];
  int16_t gyro_z_raw = buf[12] << 8 | buf[13];

  return {.accel_x = accel_x_raw / ACCEL_LSB_SENS,
          .accel_y = accel_y_raw / ACCEL_LSB_SENS,
          .accel_z = accel_z_raw / ACCEL_LSB_SENS,
          .gyro_x = gyro_x_raw / GYRO_LSB_SENS,
          .gyro_y = gyro_y_raw / GYRO_LSB_SENS,
          .gyro_z = gyro_z_raw / GYRO_LSB_SENS,
          .temp = temp_raw / TEMP_LSB_SENS + TEMP_OFFSET_C};
}

void MPU6050Device::poll(int fd)
{
  IDevice::select(fd);

  uint8_t reg = 0X3B;
  uint8_t buf[MEASURE_SIZE] = {};

  if (IDevice::write(&reg, 1, fd) < 1) {
    std::cerr << "Cannot write " << reg << "into device " << device();
    return;
  }

  if (IDevice::read(buf, MEASURE_SIZE, fd) < static_cast<ssize_t>(MEASURE_SIZE)) {
    std::cerr << "Cannot read " << MEASURE_SIZE << "into device " << device();
    return;
  }

  std::cout << to_measure(buf);
}

MPU6050Device::~MPU6050Device() {}
