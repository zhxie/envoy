#include "source/extensions/transport_sockets/tap/tap_config_impl.h"

#include "envoy/data/tap/v3/transport.pb.h"

#include "source/common/common/assert.h"
#include "source/common/network/utility.h"

#include "dml/dml.hpp"

namespace Envoy {
namespace Extensions {
namespace TransportSockets {
namespace Tap {

TapStats generateTapStats(const std::string& prefix, Stats::Scope& scope) {
  return TapStats{ALL_TAP_STATS(POOL_HISTOGRAM_PREFIX(scope, prefix))};
}

TapThreadLocal::TapThreadLocal(std::chrono::milliseconds poll_delay, Event::Dispatcher& dispatcher,
                               TapStats& stats)
    : poll_delay_(poll_delay), timer_(dispatcher.createTimer([this]() { process(); })),
      stats_(stats) {}

Buffer::InstancePtr TapThreadLocal::addRequest(Network::IoHandle& handle,
                                               Buffer::Instance& buffer) {
  // After activating write event on copy completion, pop the result.
  auto it = map_.find(handle.fdDoNotUse());
  if (it != map_.end()) {
    Buffer::InstancePtr ptr = std::move(it->second);
    RELEASE_ASSERT(ptr->length() == buffer.length(),
                   "buffer length changing behavior has not been implemented");
    map_.erase(it);
    return ptr;
  }

  // All slices in the buffer are too small to use DSA copy, use CPU copy instead.
  size_t worth_dsa = 0;
  for (const Buffer::RawSlice& slice : buffer.getRawSlices()) {
    if (slice.len_ >= MIN_BUFFER_SIZE) {
      worth_dsa += 1;
    }
  }
  if (!worth_dsa) {
    Buffer::InstancePtr copy = std::make_unique<Buffer::OwnedImpl>(buffer);
    stats_.escaped_size_.recordValue(copy->length());
    return copy;
  }

  // Add a new request and trigger DML copy.
  Request request{handle, buffer};
  queue_.push_back(request);
  queue_size_ += worth_dsa;
  if (queue_size_ >= BATCH_SIZE) {
    // There are more then BATCH_SIZE requests in the queue and we can process them.
    stopTimer();
    process();
  } else if (queue_size_ == worth_dsa) {
    // First request in the queue, start the queue timer.
    startTimer();
  }
  return nullptr;
}

void TapThreadLocal::process() {
  // Submit DSA operations.
  ENVOY_LOG(debug, "DSA process {} requests", queue_.size());
  using Handler = dml::handler<dml::mem_copy_operation, std::allocator<std::uint8_t>>;
  std::vector<Buffer::InstancePtr> buffers;
  std::vector<Operation> cpu_operations;
  std::vector<Operation> dml_operations;
  std::vector<Handler> dml_handlers;
  for (const Request& request : queue_) {
    std::unique_ptr<Buffer::OwnedImpl> buffer = std::make_unique<Buffer::OwnedImpl>();
    const Buffer::RawSliceVector raw_slices = request.buffer_.getRawSlices();
    for (const Buffer::RawSlice& src : raw_slices) {
      Buffer::Slice& dest = buffer->addEmptySlice(src.len_);
      Buffer::Slice::Reservation reservation = dest.reserve(src.len_);
      Operation operation{src, reinterpret_cast<uint8_t*>(reservation.mem_)};
      if (src.len_ < MIN_BUFFER_SIZE) {
        cpu_operations.push_back(operation);
      } else {
        dml::const_data_view src_view =
            dml::make_view(reinterpret_cast<const uint8_t*>(src.mem_), src.len_);
        dml::data_view dest_view = dml::make_view(dest.data(), src.len_);
        Handler handler = dml::submit<dml::automatic>(dml::mem_copy, src_view, dest_view);
        ASSERT(handler.valid());
        dml_handlers.push_back(std::move(handler));
        dml_operations.push_back(operation);
      }
      dest.commit(reservation);
    }

    buffers.push_back(std::move(buffer));
  }
  ENVOY_LOG(debug, "DML process {} operations and CPU process {} operations", dml_handlers.size(),
            cpu_operations.size());

  // Perform CPU operations on small slices.
  for (const Operation& operation : cpu_operations) {
    memcpy(operation.dest_, operation.src_.mem_, operation.src_.len_); // NOLINT(safe-memcpy)
    stats_.cpu_copy_size_.recordValue(operation.src_.len_);
  }

  // Wait DML operations to complete.
  for (size_t i = 0; i < dml_handlers.size(); i++) {
    Handler& handler = dml_handlers[i];
    Operation& operation = dml_operations[i];
    dml::status_code result = handler.get().status;
    if (result != dml::status_code::ok) {
      ENVOY_LOG(warn, "DML process operation from {} to {} size {} failed with result {}",
                fmt::ptr(operation.src_.mem_), fmt::ptr(operation.dest_), operation.src_.len_,
                static_cast<uint32_t>(result));
      memcpy(operation.dest_, operation.src_.mem_, operation.src_.len_); // NOLINT(safe-memcpy)
      stats_.dml_copy_error_size_.recordValue(operation.src_.len_);
    } else {
      stats_.dml_copy_size_.recordValue(operation.src_.len_);
    }
  }

  // Add copy to the map and trigger callbacks.
  for (size_t i = 0; i < queue_.size(); i++) {
    map_[queue_[i].handle_.fdDoNotUse()] = std::move(buffers[i]);
    queue_[i].handle_.activateFileEvents(Event::FileReadyType::Write);
  }

  // Clean up the queue.
  stats_.dml_pipeline_size_.recordValue(queue_.size());
  queue_.clear();
  queue_size_ = 0;
}

void TapThreadLocal::startTimer() { timer_->enableHRTimer(poll_delay_); }

void TapThreadLocal::stopTimer() { timer_->disableTimer(); }

namespace TapCommon = Extensions::Common::Tap;

PerSocketTapperImpl::PerSocketTapperImpl(SocketTapConfigSharedPtr config,
                                         const Network::Connection& connection)
    : config_(std::move(config)),
      sink_handle_(config_->createPerTapSinkHandleManager(connection.id())),
      connection_(connection), statuses_(config_->createMatchStatusVector()) {
  config_->rootMatcher().onNewStream(statuses_);
  if (config_->streaming() && config_->rootMatcher().matchStatus(statuses_).matches_) {
    // TODO(mattklein123): For IP client connections, local address will not be populated until
    // connection. We should re-emit connection information after connection so the streaming
    // trace gets the local address.
    TapCommon::TraceWrapperPtr trace = makeTraceSegment();
    fillConnectionInfo(*trace->mutable_socket_streamed_trace_segment()->mutable_connection());
    sink_handle_->submitTrace(std::move(trace));
  }
}

void PerSocketTapperImpl::fillConnectionInfo(envoy::data::tap::v3::Connection& connection) {
  if (connection_.connectionInfoProvider().localAddress() != nullptr) {
    // Local address might not be populated before a client connection is connected.
    Network::Utility::addressToProtobufAddress(*connection_.connectionInfoProvider().localAddress(),
                                               *connection.mutable_local_address());
  }
  Network::Utility::addressToProtobufAddress(*connection_.connectionInfoProvider().remoteAddress(),
                                             *connection.mutable_remote_address());
}

void PerSocketTapperImpl::closeSocket(Network::ConnectionEvent) {
  if (!config_->rootMatcher().matchStatus(statuses_).matches_) {
    return;
  }

  if (config_->streaming()) {
    TapCommon::TraceWrapperPtr trace = makeTraceSegment();
    auto& event = *trace->mutable_socket_streamed_trace_segment()->mutable_event();
    initEvent(event);
    event.mutable_closed();
    sink_handle_->submitTrace(std::move(trace));
  } else {
    makeBufferedTraceIfNeeded();
    fillConnectionInfo(*buffered_trace_->mutable_socket_buffered_trace()->mutable_connection());
    sink_handle_->submitTrace(std::move(buffered_trace_));
  }

  // Here we explicitly reset the sink_handle_ to release any sink resources and force a flush
  // of any data (e.g., files). This is not explicitly needed in production, but is needed in
  // tests to avoid race conditions due to deferred deletion. We could also do this with a stat,
  // but this seems fine in general and is simpler.
  sink_handle_.reset();
}

void PerSocketTapperImpl::initEvent(envoy::data::tap::v3::SocketEvent& event) {
  event.mutable_timestamp()->MergeFrom(Protobuf::util::TimeUtil::NanosecondsToTimestamp(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          config_->timeSource().systemTime().time_since_epoch())
          .count()));
}

void PerSocketTapperImpl::onRead(const Buffer::Instance& data, uint32_t bytes_read) {
  if (!config_->rootMatcher().matchStatus(statuses_).matches_) {
    return;
  }

  if (config_->streaming()) {
    TapCommon::TraceWrapperPtr trace = makeTraceSegment();
    auto& event = *trace->mutable_socket_streamed_trace_segment()->mutable_event();
    initEvent(event);
    TapCommon::Utility::addBufferToProtoBytes(*event.mutable_read()->mutable_data(),
                                              config_->maxBufferedRxBytes(), data,
                                              data.length() - bytes_read, bytes_read);
    sink_handle_->submitTrace(std::move(trace));
  } else {
    if (buffered_trace_ != nullptr && buffered_trace_->socket_buffered_trace().read_truncated()) {
      return;
    }

    makeBufferedTraceIfNeeded();
    auto& event = *buffered_trace_->mutable_socket_buffered_trace()->add_events();
    initEvent(event);
    ASSERT(rx_bytes_buffered_ <= config_->maxBufferedRxBytes());
    buffered_trace_->mutable_socket_buffered_trace()->set_read_truncated(
        TapCommon::Utility::addBufferToProtoBytes(*event.mutable_read()->mutable_data(),
                                                  config_->maxBufferedRxBytes() -
                                                      rx_bytes_buffered_,
                                                  data, data.length() - bytes_read, bytes_read));
    rx_bytes_buffered_ += event.read().data().as_bytes().size();
  }
}

void PerSocketTapperImpl::onWrite(const Buffer::Instance& data, uint32_t bytes_written,
                                  bool end_stream) {
  if (!config_->rootMatcher().matchStatus(statuses_).matches_) {
    return;
  }

  if (config_->streaming()) {
    TapCommon::TraceWrapperPtr trace = makeTraceSegment();
    auto& event = *trace->mutable_socket_streamed_trace_segment()->mutable_event();
    initEvent(event);
    TapCommon::Utility::addBufferToProtoBytes(*event.mutable_write()->mutable_data(),
                                              config_->maxBufferedTxBytes(), data, 0,
                                              bytes_written);
    event.mutable_write()->set_end_stream(end_stream);
    sink_handle_->submitTrace(std::move(trace));
  } else {
    if (buffered_trace_ != nullptr && buffered_trace_->socket_buffered_trace().write_truncated()) {
      return;
    }

    makeBufferedTraceIfNeeded();
    auto& event = *buffered_trace_->mutable_socket_buffered_trace()->add_events();
    initEvent(event);
    ASSERT(tx_bytes_buffered_ <= config_->maxBufferedTxBytes());
    buffered_trace_->mutable_socket_buffered_trace()->set_write_truncated(
        TapCommon::Utility::addBufferToProtoBytes(
            *event.mutable_write()->mutable_data(),
            config_->maxBufferedTxBytes() - tx_bytes_buffered_, data, 0, bytes_written));
    tx_bytes_buffered_ += event.write().data().as_bytes().size();
    event.mutable_write()->set_end_stream(end_stream);
  }
}

} // namespace Tap
} // namespace TransportSockets
} // namespace Extensions
} // namespace Envoy
