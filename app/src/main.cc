// Copyright (c) 2023 Google LLC
// SPDX-License-Identifier: Apache-2.0

#define PW_LOG_MODULE_NAME "APP"
#define PW_LOG_LEVEL PW_LOG_LEVEL_INFO

#include "async.h"

#include "pw_allocator_zephyr/sys_mem_block_allocator.h"
#include "pw_assert/assert.h"
#include "pw_log/log.h"
#include "pw_sensor/sensor.h"
#include "pw_sensor_zephyr/sensor.h"
#include "pw_status/try.h"
#include "pw_async_basic/dispatcher.h"
#include "pw_thread/thread.h"
#include "pw_thread_zephyr/options.h"

#include "sensor_test.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>

#define IMU DT_ALIAS(imu)

RTIO_DEFINE_WITH_MEMPOOL(rtio, 4, 4, 64, 64, 4);
pw::allocator::zephyr::ZephyrAllocator allocator(rtio.block_pool);

pw::sensor::SensorContext sensorContext({
    .r_ = &rtio,
    .allocator_ = &allocator,
});

SENSOR_DT_READ_IODEV(iodev, IMU, SENSOR_CHAN_ALL);

pw::async::BasicDispatcher dispatcher;

pw::thread::zephyr::StaticContextWithStack<2048> work_thread_stack;
[[maybe_unused]] pw::thread::Thread
    work_thread(pw::thread::zephyr::Options(work_thread_stack), dispatcher);

static void read_callback(int result, uint8_t *buf, uint32_t buf_len,
                          void *userdata) {
  const struct device *sensor = static_cast<const device *>(userdata);
  const struct sensor_decoder_api *decoder;

  PW_ASSERT(sensor_get_decoder(sensor, &decoder) == 0);

  uint16_t frame_count;
  PW_ASSERT(decoder->get_frame_count(buf, SENSOR_CHAN_ACCEL_XYZ, 0, &frame_count) == 0);
  PW_LOG_INFO("Frame count = %u", frame_count);
}

static void configure_sensor(pw::sensor::Sensor &imu) {
  auto configuration_result = imu.GetConfiguration();

  if (!configuration_result.ok()) {
    PW_LOG_ERROR("Failed to get configuration for accelerometer");
    return;
  }

  if (configuration_result.value().HasSampleRate(
          pw::sensor::SensorType::ACCELEROMETER)) {
    configuration_result.value().SetSampleRate(
        pw::sensor::SensorType::ACCELEROMETER, 100.0f);
  } else {
    PW_LOG_INFO("Sensor doesn't support sample rate attribute");
  }
  PW_ASSERT(imu.SetConfiguration(configuration_result.value()).ok());
}

int main() {
  PW_LOG_INFO("Hello Sensors");
  pw::sensor::zephyr::ZephyrSensor imu(DEVICE_DT_GET(IMU));

  configure_sensor(static_cast<pw::sensor::Sensor &>(imu));

  k_msleep(50);

  // Patch up RTIO with mempool
  rtio.block_pool = &rtio_block_pool;

  PW_ASSERT(RunAsyncTest(sensorContext, dispatcher).ok());

  k_msleep(50);

  // Read the sensor
  PW_LOG_INFO("Performing a plain Zephyr read");
  PW_ASSERT(sensor_read(&iodev, &rtio, (void *)DEVICE_DT_GET(IMU)) == 0);
  sensor_processing_with_callback(&rtio, read_callback);

  k_msleep(50);

  PW_ASSERT(test::sensor::Run(imu, sensorContext, dispatcher).ok());
  return 0;
}
