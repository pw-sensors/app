# Copyright (c) 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)
include(../pigweed/pw_build/pigweed.cmake)

include(../pigweed/pw_sensor/backend.cmake)
pw_set_backend(pw_sensor pw_sensor_zephyr.backend)

include(../pigweed/pw_async/backend.cmake)
pw_set_backend(pw_async.task pw_async_basic.task_backend)
pw_set_backend(pw_async.dispatcher pw_async_basic.dispatcher_backend)

include(../pigweed/pw_sync/backend.cmake)
pw_set_backend(pw_sync.binary_semaphore pw_sync_zephyr.binary_semaphore_backend)
pw_set_backend(pw_sync.interrupt_spin_lock pw_sync_zephyr.interrupt_spin_lock_backend)
pw_set_backend(pw_sync.thread_notification pw_sync_zephyr.thread_notification_backend)
pw_set_backend(pw_sync.timed_thread_notification pw_sync_zephyr.timed_thread_notification_backend)

include(../pigweed/pw_thread/backend.cmake)
pw_set_backend(pw_thread.thread pw_thread_zephyr.thread)
pw_set_backend(pw_thread.id pw_thread_zephyr.id)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(pw_sensor)
enable_language(C CXX)

target_sources(app
  PRIVATE
    src/main.cc
    src/async_test.cc
    src/sensor_test.cc
    include/async.h
    include/sensor_test.h
)

target_include_directories(app PRIVATE include)

target_link_libraries(app PRIVATE
    pw_sensor
    pw_sensor_zephyr.backend
    pw_allocator_zephyr.sys_mem_block
    pw_async_basic.task_backend
    pw_async_basic.dispatcher_backend
    pw_thread.thread
)

