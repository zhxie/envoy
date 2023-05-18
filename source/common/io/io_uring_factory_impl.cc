#include "source/common/io/io_uring_factory_impl.h"

#include "source/common/io/io_uring_worker_impl.h"

namespace Envoy {
namespace Io {

IoUringFactoryImpl::IoUringFactoryImpl(uint32_t io_uring_size, bool use_submission_queue_polling,
                                       uint32_t accept_size, uint32_t read_buffer_size,
                                       uint32_t write_timeout_ms, ThreadLocal::SlotAllocator& tls)
    : io_uring_size_(io_uring_size), use_submission_queue_polling_(use_submission_queue_polling),
      accept_size_(accept_size), read_buffer_size_(read_buffer_size),
      write_timeout_ms_(write_timeout_ms), tls_(tls) {}

OptRef<IoUringWorker> IoUringFactoryImpl::getIoUringWorker() {
  auto ret = tls_.get();
  if (ret == absl::nullopt) {
    return absl::nullopt;
  }
  return ret;
}

void IoUringFactoryImpl::onWorkerThreadInitialized() {
  tls_.set([io_uring_size = io_uring_size_,
            use_submission_queue_polling = use_submission_queue_polling_,
            accept_size = accept_size_, read_buffer_size = read_buffer_size_,
            write_timeout_ms = write_timeout_ms_](Event::Dispatcher& dispatcher) {
    return std::make_shared<IoUringWorkerImpl>(io_uring_size, use_submission_queue_polling,
                                               accept_size, read_buffer_size, write_timeout_ms,
                                               dispatcher);
  });
}

bool IoUringFactoryImpl::currentThreadRegistered() { return tls_.currentThreadRegistered(); }

} // namespace Io
} // namespace Envoy
