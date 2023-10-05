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

#pragma once

#include <zephyr/rtio/rtio.h>

#include "pw_allocator_zephyr/sys_mem_block_allocator.h"
#include "pw_function/function.h"
#include "pw_function/pointer.h"
#include "pw_sensor/sensor.h"
#include "pw_status/status.h"
#include "pw_async_basic//dispatcher.h"

pw::Status RunAsyncTest(pw::sensor::SensorContext &ctx, pw::async::BasicDispatcher &dispatcher);

namespace test {

class Request {
public:
  explicit Request(void(*submit)(struct rtio_iodev_sqe *))
      : iodev_api_{
            .submit = submit,
        },
        iodev_{
            .api = &iodev_api_,
            .iodev_sq = RTIO_MPSC_INIT(iodev_.iodev_sq),
            .data = nullptr,
        } {}

  pw::Status Submit(struct rtio *r) {
    struct rtio_sqe sqe;

    rtio_sqe_prep_read_with_pool(&sqe, &iodev_, RTIO_PRIO_NORM, nullptr);

    PW_ASSERT(rtio_sqe_copy_in_get_handles(r, &sqe, &handle_, 1) == 0);

    rtio_submit(r, 0);

    return pw::OkStatus();
  }
  struct rtio_iodev *operator&() {
    return &iodev_;
  }

private:
  const struct rtio_iodev_api iodev_api_;
  struct rtio_iodev iodev_;
  struct rtio_sqe *handle_ = nullptr;
};

} // namespace test
