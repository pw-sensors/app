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

#include "async.h"
#include "pw_assert/assert.h"
#include "pw_async_basic/dispatcher.h"
#include "pw_status/status.h"
#include "pw_thread/thread.h"

#include <zephyr/rtio/rtio.h>

namespace {

extern "C" void submit_impl(struct rtio_iodev_sqe *iodev_sqe) {
  uint8_t *buf = nullptr;
  uint32_t buf_len = 0;

  if (rtio_sqe_rx_buf(iodev_sqe, 16, 24, &buf, &buf_len) != 0) {
    printk("failed to get buffer\n");
    rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
    return;
  }

  printk("Allocated buffer @%p with size %zu\n", buf, buf_len);
  rtio_iodev_sqe_ok(iodev_sqe, 0);
}

} // namespace

pw::Status RunAsyncTest(pw::sensor::SensorContext &ctx,
                        pw::async::BasicDispatcher &dispatcher) {
  // TODO can I use pw::Function?
  test::Request iodev(submit_impl);

  iodev.Submit(ctx.native_type().r_);

  auto task_handler = [&ctx]([[maybe_unused]] pw::async::Context &c,
                             pw::Status status) {
    PW_ASSERT(status.ok());
    struct rtio_cqe *cqe = rtio_cqe_consume_block(ctx.native_type().r_);

    if (cqe->result != 0) {
      rtio_cqe_release(ctx.native_type().r_, cqe);
      return;
    }
    uint8_t *buf = nullptr;
    uint32_t buf_len = 0;
    rtio_cqe_get_mempool_buffer(ctx.native_type().r_, cqe, &buf, &buf_len);

    rtio_cqe_release(ctx.native_type().r_, cqe);

    {
      auto result = pw::sensor::Block(
          ctx.native_type().allocator_,
          pw::ByteSpan((std::byte *)buf, buf_len));

      PW_ASSERT(static_cast<void *>(result.data()) == static_cast<void *>(buf));
      PW_ASSERT(result.size() == buf_len);
    }

    PW_ASSERT(sys_mem_blocks_is_region_free(
        ctx.native_type().r_->block_pool, buf,
        buf_len >> ctx.native_type().r_->block_pool->info.blk_sz_shift));
    printk("Task is done\n");
  };
  pw::async::Task task(task_handler);
  dispatcher.Post(task);

  printk("Task posted\n");
  dispatcher.RunUntilIdle();
  printk("Dispatcher is idle\n");
  return pw::OkStatus();
}
