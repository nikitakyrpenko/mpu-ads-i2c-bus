#include "ADS1115Device.hpp"

#include <iostream>

namespace {
static constexpr int8_t MODE_CONTINOUS = 0b0;
static constexpr int8_t DATA_RATE = 0b000;  // 8 SPS
static constexpr int8_t PGA = 0b010;        // 2.048 V
static constexpr int8_t MUX = 0b100;        // AIN0 -> GND
static constexpr int8_t COMP_QUE = 0b11;    // comparator disabled

static constexpr float PGA_FSR = 2.048f;          // full-scale range for PGA = 0b010
static constexpr int ADS1115_SAMPLE_RATE_HZ = 8;  // matches DATA_RATE = 8 SPS
}  // namespace

uint16_t ADS1115Device::address()
{
  return 0x48;
}
const char* ADS1115Device::device()
{
  return "ADS1115";
}
std::chrono::milliseconds ADS1115Device::poll_interval()
{
  return std::chrono::milliseconds(10 * 1000 / ADS1115_SAMPLE_RATE_HZ);
}

bool ADS1115Device::is_alive(int fd)
{
  IDevice::select(fd);

  const uint8_t reg = 0x00;
  uint8_t conversion[2];

  if (IDevice::write(&reg, 1, fd) < 1) {
    std::cerr << "Cannot write " << reg << " into device " << device();
    return false;
  }

  if (IDevice::read(conversion, sizeof(conversion), fd) < static_cast<ssize_t>(sizeof(conversion))) {
    std::cerr << "Cannot read " << reg << " into device " << device();
    return false;
  }

  if (conversion[0] == 0) {
    std::cerr << "Device : " << device() << " conversion register reads zero, device not responding\n";
    return false;
  }

  return true;
}

bool ADS1115Device::setup(int fd)
{
  uint16_t config = (MUX << 12) | (PGA << 9) | (MODE_CONTINOUS << 8) | (DATA_RATE << 5) | COMP_QUE;
  uint8_t config_frame[3] = {0x01, static_cast<uint8_t>(config >> 8), static_cast<uint8_t>(config & 0xFF)};

  if (IDevice::write(config_frame, sizeof(config_frame), fd) < 3) {
    std::cerr << "Cannot write " << config_frame[1] << " : " << config_frame[2] << "into device " << device();
    return false;
  }

  std::cout << "Device : " << device() << " | config applied : " << "MUX : AIN0->GND | " << "PGA : +-2.048V | " << "MODE : continuous | "
            << "Data rate : 8 SPS\n";
  return true;
}

float ADS1115Device::to_voltage(uint8_t* buf)
{
  int16_t raw = buf[0] << 8 | buf[1];
  return raw * (PGA_FSR / 32768.0f);
}

void ADS1115Device::poll(int fd)
{
  IDevice::select(fd);

  const uint8_t reg = 0x00;
  uint8_t buf[2];

  if (IDevice::write(&reg, 1, fd) < 1) {
    std::cerr << "Cannot write " << reg << " into device " << device();
    return;
  }

  if (IDevice::read(buf, sizeof(buf), fd) < 2) {
    std::cerr << "Cannot read " << reg << " into device " << device();
    return;
  }

  std::cout << "Device : " << device() << " | voltage[V]: " << to_voltage(buf) << '\n';
}
