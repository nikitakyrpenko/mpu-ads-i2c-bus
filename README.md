# mpu-ads-i2c-bus

A small Linux userspace C++20 program that talks to two I2C sensors, an **ADS1115** 16-bit ADC and an **MPU-6050** accelerometer/gyroscope, and prints decoded readings to the console. It uses the plain `/dev/i2c-N` interface (`ioctl(I2C_SLAVE)`, `read`, `write`), with no third-party libraries.

Without real hardware it runs against the course's I2C simulator, `scripts/libi2csim.so`, injected with `LD_PRELOAD`.

## What it does

1. Takes the bus path and a list of device addresses on the command line.
2. Keeps only the drivers whose address was requested.
3. For each one, checks that the device really responds (`is_alive`), runs its `setup`, and drops devices that don't answer.
4. Polls the remaining devices forever, each at its own interval, and prints one decoded line per reading. Stop it with Ctrl-C.

## Devices

| Device | Address | What is read | Decoding | Poll interval |
|---|---|---|---|---|
| ADS1115 | `0x48` | conversion register (16-bit, big-endian), configured for AIN0 to GND, continuous mode, 8 SPS, comparator off | `volts = raw * 2.048 / 32768` (default range +-2.048 V) | 1250 ms (10 samples at 8 SPS) |
| MPU-6050 | `0x68` | 14 bytes from `ACCEL_XOUT_H`: accel XYZ, temperature, gyro XYZ | `accel_g = raw / 16384`, `gyro_dps = raw / 131`, `temp_C = raw / 340 + 36.53` | 300 ms |

- **Liveness check:** the MPU-6050 is confirmed by reading `WHO_AM_I` (register `0x75`, expects `0x68`).
- **MPU-6050 setup:** it is woken from sleep, given the PLL clock source and the low-pass filter, and left at its default +-2 g and +-250 deg/s ranges, which match the decode constants above.

## Code layout

```text
src/Main.cpp             argument parsing, device selection, polling loop
include/IDevice.hpp      device interface: address(), is_alive(), setup(), poll(), poll_interval()
src/IDevice.cpp          shared I2C helpers: select() via ioctl, full read/write loops
include/ADS1115Device.hpp, src/ADS1115Device.cpp
include/MPU6050Device.hpp, src/MPU6050Device.cpp
scripts/libi2csim.so     course-provided I2C simulator (LD_PRELOAD)
```

`IDevice` gives every driver the same shape, so adding a sensor means one new class and one line in `main()`. The polling loop keeps a `next_poll` timestamp per device on a `steady_clock`, so a slow device does not hold up a fast one.

## Build

```sh
cmake --preset debug
cmake --build --preset debug
```

Presets: `debug`, `relwithdebinfo`, `release` and `aarch64-debug` (cross-compile for a Raspberry Pi, Radxa or Jetson). Requires CMake 3.20+, Ninja and a C++20 compiler. The `.devcontainer` provides the toolchain.

## Run

Against the simulator:

```sh
LD_PRELOAD=$PWD/scripts/libi2csim.so ./build/debug/i2c_bus /dev/i2c-1 0x48 0x68
```

On real hardware, run the same command without `LD_PRELOAD`, using the real bus path. Addresses that match no known driver are ignored, and if none match the program exits with an error.

## About the simulator

Notes from inspecting `libi2csim.so`, which matter when debugging:

- It intercepts only `open`, `openat`, `read`, `write` and `ioctl(I2C_SLAVE / I2C_SLAVE_FORCE)`. Opening an `/dev/i2c-*` path really opens `/dev/null` underneath, so `i2cdetect` and other tools that use `I2C_FUNCS` or SMBus ioctls do not work against it.
- Selecting any address, and writing zero bytes, always succeeds. There is no ACK/NACK signal, so a classic address scan would show every address as present. A device has to be confirmed by reading real register content.
- Only `0x48` and `0x68` are implemented. The ADC conversion is recomputed on every read from the current time (a sine wave giving roughly 0.4 to 1.6 V). The MPU-6050 returns `0x68` for `WHO_AM_I` and live-computed accel and gyro values, with the temperature near 25 C.

## VS Code

- Task `CMake: configure and build` (default build task).
- Launch config `Debug i2c_bus (simulator)`, which sets `LD_PRELOAD` to the simulator and passes `/dev/i2c-1 0x48 0x68`.
- clangd reads `build/debug/compile_commands.json`.
