cmake_minimum_required(VERSION 3.8)
project(SuspensionDAQ)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp csv.cpp csv.h accel.cpp accelerometer.cpp accelerometer.h)
add_executable(SuspensionDAQ ${SOURCE_FILES})