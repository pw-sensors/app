# Copyright (c) 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)
include(../pigweed/pw_build/pigweed.cmake)
include(../pigweed/pw_sensor/backend.cmake)
pw_set_backend(pw_sensor pw_sensor_zephyr.backend)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(pw_sensor)
enable_language(C CXX)

target_sources(app
  PRIVATE
    src/main.cc
)

target_link_libraries(app PRIVATE pw_sensor pw_sensor_zephyr.backend)

