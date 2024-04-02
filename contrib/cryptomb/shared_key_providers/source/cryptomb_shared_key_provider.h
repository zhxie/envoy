#pragma once

#include "envoy/api/api.h"
#include "envoy/event/dispatcher.h"
#include "envoy/ssl/shared_key_provider.h"
#include "envoy/thread_local/thread_local.h"

#include "source/common/common/logger.h"

#include "contrib/cryptomb/private_key_providers/source/ipp_crypto.h"
#include "contrib/cryptomb/shared_key_providers/source/cryptomb_stats.h"

namespace Envoy {
namespace Extensions {
namespace SharedKeyMethodProvider {
namespace CryptoMb {

enum class RequestStatus { Retry, Success, Error };

// CryptoMbContext holds the actual data to be computed. It also has a reference
// to the worker thread dispatcher for communicating that it has has ran the
// `AVX-512` code and the result is ready to be used.
class CryptoMbContext {
public:
  static constexpr ssize_t MAX_SIGNATURE_SIZE = 512;

  CryptoMbContext(Event::Dispatcher& dispatcher, Ssl::SharedKeyConnectionCallbacks& cb);
  virtual ~CryptoMbContext() = default;

  void setStatus(RequestStatus status) { status_ = status; }
  enum RequestStatus getStatus() { return status_; }
  void scheduleCallback(enum RequestStatus status);

  virtual uint16_t groupId() const PURE;

  virtual bool init(const uint8_t* peer_key) PURE;

  // Ciphertext and secret.
  std::unique_ptr<uint8_t[]> ciphertext_;
  std::unique_ptr<uint8_t[]> secret_;
  size_t ciphertext_len_;
  size_t secret_len_;

private:
  // Whether the decryption / signing is ready.
  enum RequestStatus status_ {};

  Event::Dispatcher& dispatcher_;
  Ssl::SharedKeyConnectionCallbacks& cb_;
  // For scheduling the callback to the next dispatcher cycle.
  Event::SchedulableCallbackPtr schedulable_{};
};

// CryptoMbX25519Context is a CryptoMbContext which holds the extra X25519 parameters and has
// custom initialization function.
class CryptoMbX25519Context : public CryptoMbContext {
public:
  CryptoMbX25519Context(Event::Dispatcher& dispatcher, Ssl::SharedKeyConnectionCallbacks& cb)
      : CryptoMbContext(dispatcher, cb) {}
  bool init(const uint8_t* peer_key) override;

  uint16_t groupId() const override { return SSL_GROUP_X25519; };

  bool initialized_{};
  // Peer key.
  uint8_t peer_key_[32];
  // Private key.
  uint8_t private_key_[32];
};

// CryptoMbP256Context is a CryptoMbContext which holds the extra P-256 parameters and has custom
// initialization function.
class CryptoMbP256Context : public CryptoMbContext {
public:
  CryptoMbP256Context(Event::Dispatcher& dispatcher, Ssl::SharedKeyConnectionCallbacks& cb)
      : CryptoMbContext(dispatcher, cb) {}
  bool init(const uint8_t* peer_key) override;

  uint16_t groupId() const override { return SSL_GROUP_SECP256R1; };

  bool initialized_{};
  // Peer key.
  bssl::UniquePtr<BIGNUM> peer_x_;
  bssl::UniquePtr<BIGNUM> peer_y_;
  // Private key.
  bssl::UniquePtr<BIGNUM> private_key_;
  // Public key.
  bssl::UniquePtr<BIGNUM> public_x_;
  bssl::UniquePtr<BIGNUM> public_y_;
};

using CryptoMbContextSharedPtr = std::shared_ptr<CryptoMbContext>;
using CryptoMbX25519ContextSharedPtr = std::shared_ptr<CryptoMbX25519Context>;
using CryptoMbP256ContextSharedPtr = std::shared_ptr<CryptoMbP256Context>;

// CryptoMbQueue maintains the request queue and is able to process it.
class CryptoMbQueue : public Logger::Loggable<Logger::Id::connection> {
public:
  static constexpr uint32_t MULTIBUFF_BATCH = 8;

  CryptoMbQueue(std::chrono::milliseconds poll_delay,
                PrivateKeyMethodProvider::CryptoMb::IppCryptoSharedPtr ipp, Event::Dispatcher& d,
                CryptoMbStats& stats);
  virtual ~CryptoMbQueue() = default;
  void addAndProcessEightRequests(CryptoMbContextSharedPtr mb_ctx);
  const std::chrono::microseconds& getPollDelayForTest() const { return us_; }

protected:
  virtual void processRequests() PURE;
  void startTimer();
  void stopTimer();

  // Polling delay.
  std::chrono::microseconds us_{};

  // Queue for the requests.
  std::vector<CryptoMbContextSharedPtr> request_queue_;

  // Thread local data slot.
  ThreadLocal::SlotPtr slot_{};

  // Crypto operations library interface.
  PrivateKeyMethodProvider::CryptoMb::IppCryptoSharedPtr ipp_{};

  // Timer to trigger queue processing if eight requests are not received in time.
  Event::TimerPtr timer_{};

  CryptoMbStats& stats_;
};

class CryptoMbX25519Queue : public CryptoMbQueue {
public:
  CryptoMbX25519Queue(std::chrono::milliseconds poll_delay,
                      PrivateKeyMethodProvider::CryptoMb::IppCryptoSharedPtr ipp,
                      Event::Dispatcher& d, CryptoMbStats& stats)
      : CryptoMbQueue(poll_delay, ipp, d, stats) {}

private:
  void processRequests() override;
};

class CryptoMbP256Queue : public CryptoMbQueue {
public:
  CryptoMbP256Queue(std::chrono::milliseconds poll_delay,
                    PrivateKeyMethodProvider::CryptoMb::IppCryptoSharedPtr ipp,
                    Event::Dispatcher& d, CryptoMbStats& stats)
      : CryptoMbQueue(poll_delay, ipp, d, stats) {}

private:
  void processRequests() override;
};

// CryptoMbSharedKeyConnection maintains the data needed by a given SSL
// connection.
class CryptoMbSharedKeyConnection : public Logger::Loggable<Logger::Id::connection> {
public:
  CryptoMbSharedKeyConnection(Ssl::SharedKeyConnectionCallbacks& cb, Event::Dispatcher& dispatcher,
                              CryptoMbX25519Queue& x25519_queue, CryptoMbP256Queue& p256_queue);
  virtual ~CryptoMbSharedKeyConnection() = default;

  void logDebugMsg(std::string msg) { ENVOY_LOG(debug, "CryptoMb: {}", msg); }
  void logWarnMsg(std::string msg) { ENVOY_LOG(warn, "CryptoMb: {}", msg); }
  void x25519AddToQueue(CryptoMbX25519ContextSharedPtr mb_ctx);
  void p256AddToQueue(CryptoMbP256ContextSharedPtr mb_ctx);

  CryptoMbX25519Queue& x25519_queue_;
  CryptoMbP256Queue& p256_queue_;
  Event::Dispatcher& dispatcher_;
  Ssl::SharedKeyConnectionCallbacks& cb_;
  CryptoMbContextSharedPtr mb_ctx_{};

private:
  Event::FileEventPtr ssl_async_event_{};
};

// CryptoMbSharedKeyMethodProvider handles the shared key method operations for
// an SSL socket.
class CryptoMbSharedKeyMethodProvider : public virtual Ssl::SharedKeyMethodProvider,
                                        public Logger::Loggable<Logger::Id::connection> {
public:
  CryptoMbSharedKeyMethodProvider(ThreadLocal::SlotAllocator& tls, Stats::Scope& stats_scope,
                                  PrivateKeyMethodProvider::CryptoMb::IppCryptoSharedPtr ipp,
                                  std::chrono::milliseconds poll_delay);

  // Ssl::SharedKeyMethodProvider
  void registerSharedKeyMethod(SSL* ssl, Ssl::SharedKeyConnectionCallbacks& cb,
                               Event::Dispatcher& dispatcher) override;
  void unregisterSharedKeyMethod(SSL* ssl) override;
  bool checkFips() override;
  bool isAvailable() override;
  Ssl::BoringSslSharedKeyMethodSharedPtr getBoringSslSharedKeyMethod() override;

  static int connectionIndex();

private:
  // Thread local data containing a single queue per worker thread.
  struct ThreadLocalData : public ThreadLocal::ThreadLocalObject {
    ThreadLocalData(std::chrono::milliseconds poll_delay,
                    PrivateKeyMethodProvider::CryptoMb::IppCryptoSharedPtr ipp,
                    Event::Dispatcher& d, CryptoMbStats& stats)
        : x25519_queue_(poll_delay, ipp, d, stats), p256_queue_(poll_delay, ipp, d, stats){};
    CryptoMbX25519Queue x25519_queue_;
    CryptoMbP256Queue p256_queue_;
  };

  Ssl::BoringSslSharedKeyMethodSharedPtr method_{};

  ThreadLocal::TypedSlotPtr<ThreadLocalData> tls_;

  CryptoMbStats stats_;

  bool initialized_{};
};

} // namespace CryptoMb
} // namespace SharedKeyMethodProvider
} // namespace Extensions
} // namespace Envoy
