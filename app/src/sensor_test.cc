// Copyright 2023 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#define PW_LOG_MODULE_NAME "SensorTest"
#define PW_LOG_LEVEL PW_LOG_LEVEL_INFO

#include <zephyr/drivers/sensor_data_types.h>

#include "sensor_test.h"

#include "pw_log/log.h"

pw::Status test::sensor::Run(pw::sensor::Sensor &sensor,
                             pw::sensor::SensorContext &ctx,
                             pw::async::BasicDispatcher &dispatcher) {
  pw::sensor::SensorType read_list[] = {pw::sensor::SensorType::ACCELEROMETER};
  auto future = sensor.Read(ctx, {read_list});
  pw::async::Task task([&future](pw::async::Context &task_context,
                                 pw::Status status) {
    if (status.IsCancelled()) {
      PW_LOG_INFO("Canceled request");
      return;
    }
    PW_LOG_INFO("Creating waker with Task@%p", task_context.task);
    pw::async::Waker waker(*task_context.dispatcher, *task_context.task);
    auto result = future.Poll(waker);
    if (result.IsReady()) {
      PW_LOG_INFO("Got result @ %p with length=%u",
                  result.value().value().data(), result.value().value().size());
      auto decoder = future.sensor()->GetDecoder();
      pw::sensor::DecoderContext decoder_context(result.value().value().as_byte_span());

      auto size_info =
          decoder->GetSizeInfo(decoder_context, pw::sensor::ACCELEROMETER);
      PW_ASSERT_OK(size_info.status());
      PW_LOG_INFO("Decoding an ACCELEROMETER requires %zu bytes for the first "
                  "frame and %zu bytes for every additional frame",
                  size_info.value().base_size, size_info.value().frame_size);

      auto frame_count =
          decoder->GetFrameCount(decoder_context, pw::sensor::ACCELEROMETER, 0);
      PW_ASSERT_OK(frame_count.status());
      PW_LOG_INFO("Found %zu ACCELEROMETER frames", frame_count.value());

      std::byte out[64];
      auto decode_result = decoder->Decode(
          decoder_context, pw::sensor::ACCELEROMETER, 0, 1, pw::ByteSpan(out));
      PW_ASSERT_OK(decode_result.status());
      PW_ASSERT(decode_result.value() == 1);

      struct sensor_three_axis_data *data =
          (struct sensor_three_axis_data *)out;
      PW_LOG_INFO("Reading:");
      PW_LOG_INFO("  timestamp: %" PRIu64, data->header.base_timestamp_ns);
      PW_LOG_INFO("  reading_count: %" PRIu16, data->header.reading_count);
      PW_LOG_INFO("  [0] %" PRIsensor_three_axis_data,
                  PRIsensor_three_axis_data_arg(*data, 0));
    }
  });
  PW_LOG_INFO("Task@%p, Future@%p", &task, &future);
  dispatcher.Post(task);
  dispatcher.RunUntilIdle();
  PW_LOG_INFO("Dispatcher is idle");
  return pw::OkStatus();
}
