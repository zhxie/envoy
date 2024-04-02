#include "contrib/cryptomb/shared_key_providers/source/cryptomb_shared_key_provider.h"

#include <memory>

#include "envoy/registry/registry.h"
#include "envoy/server/transport_socket_config.h"

#include "source/common/config/datasource.h"

#include "openssl/rand.h"
#include "openssl/ssl.h"

namespace Envoy {
namespace Extensions {
namespace SharedKeyMethodProvider {
namespace CryptoMb {

CryptoMbContext::CryptoMbContext(Event::Dispatcher& dispatcher,
                                 Ssl::SharedKeyConnectionCallbacks& cb)
    : status_(RequestStatus::Retry), dispatcher_(dispatcher), cb_(cb) {}

void CryptoMbContext::scheduleCallback(enum RequestStatus status) {
  schedulable_ = dispatcher_.createSchedulableCallback([this, status]() {
    // The status can't be set beforehand, because the callback asserts
    // if someone else races to call doHandshake() and the status goes to
    // HandshakeComplete.
    setStatus(status);
    this->cb_.onSharedKeyMethodComplete();
  });
  schedulable_->scheduleCallbackNextIteration();
}

bool CryptoMbX25519Context::init(const uint8_t* peer_key) {
  if (initialized_) {
    return false;
  }

  ciphertext_ = std::make_unique<uint8_t[]>(32);
  ciphertext_len_ = 32;
  secret_ = std::make_unique<uint8_t[]>(32);
  secret_len_ = 32;

  memcpy(peer_key_, peer_key, 32); // NOLINT(safe-memcpy)

  RAND_bytes(private_key_, 32);
  private_key_[0] |= ~248;
  private_key_[31] &= ~64;
  private_key_[31] |= ~127;

  initialized_ = true;

  return true;
}

bool CryptoMbP256Context::init(const uint8_t* peer_key) {
  if (initialized_) {
    return false;
  }

  ciphertext_ = std::make_unique<uint8_t[]>(65);
  ciphertext_len_ = 65;
  secret_ = std::make_unique<uint8_t[]>(32);
  secret_len_ = 32;

  peer_x_.reset(BN_new());
  peer_y_.reset(BN_new());
  if (!peer_x_ || !peer_y_) {
    return false;
  }
  BN_bin2bn(&peer_key[1], 32, peer_x_.get());
  BN_bin2bn(&peer_key[33], 32, peer_y_.get());

  private_key_.reset(BN_new());
  if (!private_key_ ||
      !BN_rand_range_ex(private_key_.get(), 1, EC_GROUP_get0_order(EC_group_p256()))) {
    return false;
  }

  public_x_.reset(BN_new());
  public_y_.reset(BN_new());
  if (!public_x_ || !public_y_) {
    return false;
  }

  initialized_ = true;

  return true;
}

namespace {
ssl_shared_key_result_t sharedKeyComputeInternal(CryptoMbSharedKeyConnection* ops,
                                                 uint16_t group_id, const uint8_t* peer_key,
                                                 size_t peer_key_len) {
  if (ops == nullptr) {
    return ssl_shared_key_failure;
  }

  switch (group_id) {
  case SSL_GROUP_X25519: {
    if (peer_key_len != 32) {
      return ssl_shared_key_failure;
    }

    CryptoMbX25519ContextSharedPtr mb_ctx =
        std::make_shared<CryptoMbX25519Context>(ops->dispatcher_, ops->cb_);

    if (!mb_ctx->init(peer_key)) {
      return ssl_shared_key_failure;
    }

    ops->x25519AddToQueue(mb_ctx);
    return ssl_shared_key_retry;
  }
  case SSL_GROUP_SECP256R1: {
    if (peer_key_len != 65 || peer_key[0] != POINT_CONVERSION_UNCOMPRESSED) {
      return ssl_shared_key_failure;
    }

    CryptoMbP256ContextSharedPtr mb_ctx =
        std::make_shared<CryptoMbP256Context>(ops->dispatcher_, ops->cb_);

    if (!mb_ctx->init(peer_key)) {
      return ssl_shared_key_failure;
    }

    ops->p256AddToQueue(mb_ctx);
    return ssl_shared_key_retry;
  }
  default:
    return ssl_shared_key_failure;
  }
}

ssl_shared_key_result_t sharedKeyCompute(SSL* ssl, uint16_t group_id, const uint8_t* peer_key,
                                         size_t peer_key_len) {
  return ssl == nullptr ? ssl_shared_key_failure
                        : sharedKeyComputeInternal(
                              static_cast<CryptoMbSharedKeyConnection*>(SSL_get_ex_data(
                                  ssl, CryptoMbSharedKeyMethodProvider::connectionIndex())),
                              group_id, peer_key, peer_key_len);
}

ssl_shared_key_result_t sharedKeyCompleteInternal(CryptoMbSharedKeyConnection* ops,
                                                  uint8_t** ciphertext, size_t* ciphertext_len,
                                                  uint8_t** secret, size_t* secret_len) {
  if (ops == nullptr) {
    return ssl_shared_key_failure;
  }

  // Check if the MB operation is ready yet. This can happen if someone calls
  // the top-level SSL function too early. The op status is only set from this
  // thread.
  if (ops->mb_ctx_->getStatus() == RequestStatus::Retry) {
    return ssl_shared_key_retry;
  }

  // If this point is reached, the MB processing must be complete.

  // See if the operation failed.
  if (ops->mb_ctx_->getStatus() != RequestStatus::Success) {
    ops->logWarnMsg("shared key operation failed.");
    return ssl_shared_key_failure;
  }

  switch (ops->mb_ctx_->groupId()) {
  case SSL_GROUP_SECP256R1: {
    CryptoMbP256ContextSharedPtr mb_ctx =
        std::static_pointer_cast<CryptoMbP256Context>(ops->mb_ctx_);
    mb_ctx->ciphertext_[0] = 4;
    if (!BN_bn2bin_padded(&mb_ctx->ciphertext_.get()[1], 32, mb_ctx->public_x_.get()) ||
        !BN_bn2bin_padded(&mb_ctx->ciphertext_.get()[33], 32, mb_ctx->public_y_.get())) {
      return ssl_shared_key_failure;
    }
  } break;
  default:
    break;
  }

  *ciphertext = ops->mb_ctx_->ciphertext_.get();
  *secret = ops->mb_ctx_->secret_.get();
  *ciphertext_len = ops->mb_ctx_->ciphertext_len_;
  *secret_len = ops->mb_ctx_->secret_len_;

  return ssl_shared_key_success;
}

ssl_shared_key_result_t sharedKeyComplete(SSL* ssl, uint8_t** ciphertext, size_t* ciphertext_len,
                                          uint8_t** secret, size_t* secret_len) {
  return ssl == nullptr ? ssl_shared_key_failure
                        : sharedKeyCompleteInternal(
                              static_cast<CryptoMbSharedKeyConnection*>(SSL_get_ex_data(
                                  ssl, CryptoMbSharedKeyMethodProvider::connectionIndex())),
                              ciphertext, ciphertext_len, secret, secret_len);
}
} // namespace

CryptoMbQueue::CryptoMbQueue(std::chrono::milliseconds poll_delay,
                             PrivateKeyMethodProvider::CryptoMb::IppCryptoSharedPtr ipp,
                             Event::Dispatcher& d, CryptoMbStats& stats)
    : us_(std::chrono::duration_cast<std::chrono::microseconds>(poll_delay)), ipp_(ipp),
      timer_(d.createTimer([this]() { processRequests(); })), stats_(stats) {
  request_queue_.reserve(MULTIBUFF_BATCH);
}

void CryptoMbQueue::startTimer() { timer_->enableHRTimer(us_); }

void CryptoMbQueue::stopTimer() { timer_->disableTimer(); }

void CryptoMbQueue::addAndProcessEightRequests(CryptoMbContextSharedPtr mb_ctx) {
  // Add the request to the processing queue.
  ASSERT(request_queue_.size() < MULTIBUFF_BATCH);
  request_queue_.push_back(mb_ctx);

  if (request_queue_.size() == MULTIBUFF_BATCH) {
    // There are eight requests in the queue and we can process them.
    stopTimer();
    ENVOY_LOG(debug, "processing directly 8 requests");
    processRequests();
  } else if (request_queue_.size() == 1) {
    // First request in the queue, start the queue timer.
    startTimer();
  }
}

void CryptoMbX25519Queue::processRequests() {
  stats_.x25519_queue_sizes_.recordValue(request_queue_.size());

  uint8_t* pa_public_key[MULTIBUFF_BATCH] = {};
  const uint8_t* pa_private_key[MULTIBUFF_BATCH] = {};
  uint8_t* pa_shared_key[MULTIBUFF_BATCH] = {};
  const uint8_t* pa_peer_key[MULTIBUFF_BATCH] = {};
  for (unsigned req_num = 0; req_num < request_queue_.size(); req_num++) {
    CryptoMbX25519ContextSharedPtr mb_ctx =
        std::static_pointer_cast<CryptoMbX25519Context>(request_queue_[req_num]);
    pa_public_key[req_num] = mb_ctx->ciphertext_.get();
    pa_private_key[req_num] = mb_ctx->private_key_;
  }

  ENVOY_LOG(debug, "Multibuffer X25519 process {} requests", request_queue_.size());

  uint32_t x25519_sts = ipp_->mbxX25519PublicKeyMb8(pa_public_key, pa_private_key);

  enum RequestStatus status[MULTIBUFF_BATCH] = {RequestStatus::Retry};
  for (unsigned req_num = 0; req_num < request_queue_.size(); req_num++) {
    CryptoMbX25519ContextSharedPtr mb_ctx =
        std::static_pointer_cast<CryptoMbX25519Context>(request_queue_[req_num]);
    if (ipp_->mbxGetSts(x25519_sts, req_num)) {
      ENVOY_LOG(debug, "Multibuffer X25519 request {} success", req_num);
      status[req_num] = RequestStatus::Success;
    } else {
      ENVOY_LOG(debug, "Multibuffer X25519 request {} failure", req_num);
      status[req_num] = RequestStatus::Error;
    }

    pa_shared_key[req_num] = mb_ctx->secret_.get();
    pa_peer_key[req_num] = mb_ctx->peer_key_;
  }

  x25519_sts = ipp_->mbxX25519Mb8(pa_shared_key, pa_private_key, pa_peer_key);

  for (unsigned req_num = 0; req_num < request_queue_.size(); req_num++) {
    CryptoMbX25519ContextSharedPtr mb_ctx =
        std::static_pointer_cast<CryptoMbX25519Context>(request_queue_[req_num]);
    enum RequestStatus ctx_status;
    if (!ipp_->mbxGetSts(x25519_sts, req_num)) {
      status[req_num] = RequestStatus::Error;
    }

    ctx_status = status[req_num];
    mb_ctx->scheduleCallback(ctx_status);
  }

  request_queue_.clear();
}

void CryptoMbP256Queue::processRequests() {
  stats_.p256_queue_sizes_.recordValue(request_queue_.size());

  BIGNUM* pa_public_x[MULTIBUFF_BATCH] = {};
  BIGNUM* pa_public_y[MULTIBUFF_BATCH] = {};
  const BIGNUM* pa_private_key[MULTIBUFF_BATCH] = {};
  uint8_t* pa_shared_key[MULTIBUFF_BATCH] = {};
  const BIGNUM* pa_peer_x[MULTIBUFF_BATCH] = {};
  const BIGNUM* pa_peer_y[MULTIBUFF_BATCH] = {};
  for (unsigned req_num = 0; req_num < request_queue_.size(); req_num++) {
    CryptoMbP256ContextSharedPtr mb_ctx =
        std::static_pointer_cast<CryptoMbP256Context>(request_queue_[req_num]);
    pa_public_x[req_num] = mb_ctx->public_x_.get();
    pa_public_y[req_num] = mb_ctx->public_y_.get();
    pa_private_key[req_num] = mb_ctx->private_key_.get();
  }

  ENVOY_LOG(debug, "Multibuffer P-256 process {} requests", request_queue_.size());

  uint32_t p256_sts = ipp_->mbxNistp256EcpublicKeySslMb8(pa_public_x, pa_public_y, pa_private_key);

  enum RequestStatus status[MULTIBUFF_BATCH] = {RequestStatus::Retry};
  for (unsigned req_num = 0; req_num < request_queue_.size(); req_num++) {
    CryptoMbP256ContextSharedPtr mb_ctx =
        std::static_pointer_cast<CryptoMbP256Context>(request_queue_[req_num]);
    if (ipp_->mbxGetSts(p256_sts, req_num)) {
      ENVOY_LOG(debug, "Multibuffer P256 request {} success", req_num);
      status[req_num] = RequestStatus::Success;
    } else {
      ENVOY_LOG(debug, "Multibuffer P256 request {} failure", req_num);
      status[req_num] = RequestStatus::Error;
    }

    pa_shared_key[req_num] = mb_ctx->secret_.get();
    pa_peer_x[req_num] = mb_ctx->peer_x_.get();
    pa_peer_y[req_num] = mb_ctx->peer_y_.get();
  }

  p256_sts = ipp_->mbxNistp256EcdhSslMb8(pa_shared_key, pa_private_key, pa_peer_x, pa_peer_y);

  for (unsigned req_num = 0; req_num < request_queue_.size(); req_num++) {
    CryptoMbP256ContextSharedPtr mb_ctx =
        std::static_pointer_cast<CryptoMbP256Context>(request_queue_[req_num]);
    enum RequestStatus ctx_status;
    if (!ipp_->mbxGetSts(p256_sts, req_num)) {
      status[req_num] = RequestStatus::Error;
    }

    ctx_status = status[req_num];
    mb_ctx->scheduleCallback(ctx_status);
  }

  request_queue_.clear();
}

CryptoMbSharedKeyConnection::CryptoMbSharedKeyConnection(Ssl::SharedKeyConnectionCallbacks& cb,
                                                         Event::Dispatcher& dispatcher,
                                                         CryptoMbX25519Queue& x25519_queue,
                                                         CryptoMbP256Queue& p256_queue)
    : x25519_queue_(x25519_queue), p256_queue_(p256_queue), dispatcher_(dispatcher), cb_(cb) {}

void CryptoMbSharedKeyMethodProvider::registerSharedKeyMethod(SSL* ssl,
                                                              Ssl::SharedKeyConnectionCallbacks& cb,
                                                              Event::Dispatcher& dispatcher) {

  if (SSL_get_ex_data(ssl, CryptoMbSharedKeyMethodProvider::connectionIndex()) != nullptr) {
    throw EnvoyException("Not registering the CryptoMb provider twice for same context");
  }

  ASSERT(tls_->currentThreadRegistered(), "Current thread needs to be registered.");

  CryptoMbX25519Queue& x25519_queue = tls_->get()->x25519_queue_;
  CryptoMbP256Queue& p256_queue = tls_->get()->p256_queue_;
  CryptoMbSharedKeyConnection* ops =
      new CryptoMbSharedKeyConnection(cb, dispatcher, x25519_queue, p256_queue);

  SSL_set_ex_data(ssl, CryptoMbSharedKeyMethodProvider::connectionIndex(), ops);
}

void CryptoMbSharedKeyConnection::x25519AddToQueue(CryptoMbX25519ContextSharedPtr mb_ctx) {
  mb_ctx_ = mb_ctx;
  x25519_queue_.addAndProcessEightRequests(mb_ctx_);
}

void CryptoMbSharedKeyConnection::p256AddToQueue(CryptoMbP256ContextSharedPtr mb_ctx) {
  mb_ctx_ = mb_ctx;
  p256_queue_.addAndProcessEightRequests(mb_ctx_);
}

bool CryptoMbSharedKeyMethodProvider::checkFips() {
  // `ipp-crypto` library is not fips-certified at the moment
  // (https://github.com/intel/ipp-crypto#certification).
  return false;
}

bool CryptoMbSharedKeyMethodProvider::isAvailable() { return initialized_; }

Ssl::BoringSslSharedKeyMethodSharedPtr
CryptoMbSharedKeyMethodProvider::getBoringSslSharedKeyMethod() {
  return method_;
}

void CryptoMbSharedKeyMethodProvider::unregisterSharedKeyMethod(SSL* ssl) {
  CryptoMbSharedKeyConnection* ops = static_cast<CryptoMbSharedKeyConnection*>(
      SSL_get_ex_data(ssl, CryptoMbSharedKeyMethodProvider::connectionIndex()));
  SSL_set_ex_data(ssl, CryptoMbSharedKeyMethodProvider::connectionIndex(), nullptr);
  delete ops;
}

// The CryptoMbSharedKeyMethodProvider is created on config.
CryptoMbSharedKeyMethodProvider::CryptoMbSharedKeyMethodProvider(
    ThreadLocal::SlotAllocator& tls, Stats::Scope& stats_scope,
    PrivateKeyMethodProvider::CryptoMb::IppCryptoSharedPtr ipp,
    std::chrono::milliseconds poll_delay)
    : tls_(ThreadLocal::TypedSlot<ThreadLocalData>::makeUnique(tls)),
      stats_(generateCryptoMbStats("cryptomb", stats_scope)) {

  if (!ipp->mbxIsCryptoMbApplicable(0)) {
    throw EnvoyException("Multi-buffer CPU instructions not available.");
  }

  method_ = std::make_shared<SSL_SHARED_KEY_METHOD>();
  method_->compute = sharedKeyCompute;
  method_->complete = sharedKeyComplete;

  // Create a single queue for every worker thread to avoid locking.
  tls_->set([poll_delay, ipp, this](Event::Dispatcher& d) {
    ENVOY_LOG(debug, "Created CryptoMb Queue for thread {}", d.name());
    return std::make_shared<ThreadLocalData>(poll_delay, ipp, d, stats_);
  });

  initialized_ = true;
}

namespace {
int createIndex() {
  int index = SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
  RELEASE_ASSERT(index >= 0, "Failed to get SSL user data index.");
  return index;
}
} // namespace

int CryptoMbSharedKeyMethodProvider::connectionIndex() {
  CONSTRUCT_ON_FIRST_USE(int, createIndex());
}

} // namespace CryptoMb
} // namespace SharedKeyMethodProvider
} // namespace Extensions
} // namespace Envoy
