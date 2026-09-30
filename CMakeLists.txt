cmake_minimum_required(VERSION 3.20)

project(i2c_bus CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(i2c_bus src/Main.cpp src/IDevice.cpp src/ADS1115Device.cpp src/MPU6050Device.cpp)
target_compile_options(i2c_bus PRIVATE -Wall -Wextra -pedantic)
target_include_directories(i2c_bus PRIVATE include)
