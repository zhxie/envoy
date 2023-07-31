#pragma once

#include "envoy/config/tap/v3/common.pb.h"
#include "envoy/data/tap/v3/transport.pb.h"
#include "envoy/event/timer.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"

#include "source/extensions/common/tap/tap_config_base.h"
#include "source/extensions/transport_sockets/tap/tap_config.h"

namespace Envoy {
namespace Extensions {
namespace TransportSockets {
namespace Tap {

#define ALL_TAP_STATS(HISTOGRAM)                                                                   \
  HISTOGRAM(escaped_size, Bytes)                                                                   \
  HISTOGRAM(unaligned_size, Bytes)                                                                 \
  HISTOGRAM(cpu_copy_size, Bytes)                                                                  \
  HISTOGRAM(dml_pipeline_size, Unspecified)                                                        \
  HISTOGRAM(dml_copy_size, Bytes)                                                                  \
  HISTOGRAM(dml_copy_error_size, Bytes)

struct TapStats {
  ALL_TAP_STATS(GENERATE_HISTOGRAM_STRUCT)
};

TapStats generateTapStats(const std::string& prefix, Stats::Scope& scope);

class TapThreadLocal : public ThreadLocal::ThreadLocalObject,
                       public Logger::Loggable<Logger::Id::tap> {
public:
  TapThreadLocal(std::chrono::milliseconds poll_delay, Event::Dispatcher& dispatcher,
                 TapStats& stats);

  Buffer::InstancePtr addRequest(Network::IoHandle& handle, Buffer::Instance& buffer);

private:
  static constexpr size_t MIN_BUFFER_SIZE = 8192;
  static constexpr size_t BATCH_SIZE = 128;

  struct Request {
    Network::IoHandle& handle_;
    Buffer::Instance& buffer_;
  };
  struct Operation {
    const Buffer::RawSlice& src_;
    uint8_t* dest_;
  };

  std::chrono::milliseconds poll_delay_;
  Event::TimerPtr timer_;
  TapStats& stats_;
  std::vector<Request> queue_{};
  size_t queue_size_{};
  absl::flat_hash_map<os_fd_t, Buffer::InstancePtr> map_{};

  void process();
  void startTimer();
  void stopTimer();
};

class PerSocketTapperImpl : public PerSocketTapper {
public:
  PerSocketTapperImpl(SocketTapConfigSharedPtr config, const Network::Connection& connection);

  // PerSocketTapper
  void closeSocket(Network::ConnectionEvent event) override;
  void onRead(const Buffer::Instance& data, uint32_t bytes_read) override;
  void onWrite(const Buffer::Instance& data, uint32_t bytes_written, bool end_stream) override;

private:
  void initEvent(envoy::data::tap::v3::SocketEvent&);
  void fillConnectionInfo(envoy::data::tap::v3::Connection& connection);
  void makeBufferedTraceIfNeeded() {
    if (buffered_trace_ == nullptr) {
      buffered_trace_ = Extensions::Common::Tap::makeTraceWrapper();
      buffered_trace_->mutable_socket_buffered_trace()->set_trace_id(connection_.id());
    }
  }
  Extensions::Common::Tap::TraceWrapperPtr makeTraceSegment() {
    Extensions::Common::Tap::TraceWrapperPtr trace = Extensions::Common::Tap::makeTraceWrapper();
    trace->mutable_socket_streamed_trace_segment()->set_trace_id(connection_.id());
    return trace;
  }

  SocketTapConfigSharedPtr config_;
  Extensions::Common::Tap::PerTapSinkHandleManagerPtr sink_handle_;
  const Network::Connection& connection_;
  Extensions::Common::Tap::Matcher::MatchStatusVector statuses_;
  // Must be a shared_ptr because of submitTrace().
  Extensions::Common::Tap::TraceWrapperPtr buffered_trace_;
  uint32_t rx_bytes_buffered_{};
  uint32_t tx_bytes_buffered_{};
};

class SocketTapConfigImpl : public Extensions::Common::Tap::TapConfigBaseImpl,
                            public SocketTapConfig,
                            public std::enable_shared_from_this<SocketTapConfigImpl> {
public:
  SocketTapConfigImpl(const envoy::config::tap::v3::TapConfig& proto_config,
                      Extensions::Common::Tap::Sink* admin_streamer, TimeSource& time_system,
                      ThreadLocal::SlotAllocator& tls, std::chrono::milliseconds poll_delay,
                      Stats::Scope& scope)
      : Extensions::Common::Tap::TapConfigBaseImpl(std::move(proto_config), admin_streamer),
        time_source_(time_system), tls_(ThreadLocal::TypedSlot<TapThreadLocal>::makeUnique(tls)),
        stats_(generateTapStats("tap", scope)) {
    tls_->set([this, poll_delay](Event::Dispatcher& dispatcher) {
      return std::make_shared<TapThreadLocal>(poll_delay, dispatcher, stats_);
    });
  }

  // SocketTapConfig
  PerSocketTapperPtr createPerSocketTapper(const Network::Connection& connection) override {
    return std::make_unique<PerSocketTapperImpl>(shared_from_this(), connection);
  }
  TimeSource& timeSource() const override { return time_source_; }

  Buffer::InstancePtr addRequest(Network::IoHandle& handle, Buffer::Instance& buffer) override {
    return tls_->get()->addRequest(handle, buffer);
  }

private:
  TimeSource& time_source_;
  ThreadLocal::TypedSlotPtr<TapThreadLocal> tls_;
  TapStats stats_;

  friend class PerSocketTapperImpl;
};

} // namespace Tap
} // namespace TransportSockets
} // namespace Extensions
} // namespace Envoy
