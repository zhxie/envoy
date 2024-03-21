#pragma once

#include <functional>
#include <string>

#include "envoy/common/pure.h"
#include "envoy/config/typed_config.h"
#include "envoy/event/dispatcher.h"
#include "envoy/server/factory_context.h"

#include "openssl/ssl.h"

namespace Envoy {

namespace Ssl {

class SharedKeyConnectionCallbacks {
public:
  virtual ~SharedKeyConnectionCallbacks() = default;

  /**
   * Callback function which is called when the asynchronous shared key
   * operation has been completed (with either success or failure). The
   * provider will communicate the success status when SSL_do_handshake()
   * is called the next time.
   */
  virtual void onSharedKeyMethodComplete() PURE;
};

#ifdef OPENSSL_IS_BORINGSSL
using BoringSslSharedKeyMethodSharedPtr = std::shared_ptr<SSL_SHARED_KEY_METHOD>;
#endif

class SharedKeyMethodProvider {
public:
  virtual ~SharedKeyMethodProvider() = default;

  /**
   * Register an SSL connection to shared key operations by the provider.
   * @param ssl a SSL connection object.
   * @param cb a callbacks object, whose "complete" method will be invoked
   * when the asynchronous processing is complete.
   * @param dispatcher supplies the owning thread's dispatcher.
   */
  virtual void registerSharedKeyMethod(SSL* ssl, SharedKeyConnectionCallbacks& cb,
                                       Event::Dispatcher& dispatcher) PURE;

  /**
   * Unregister an SSL connection from shared key operations by the provider.
   * @param ssl a SSL connection object.
   * @throw EnvoyException if registration fails.
   */
  virtual void unregisterSharedKeyMethod(SSL* ssl) PURE;

  /**
   * Check whether the shared key method satisfies FIPS requirements.
   * @return true if FIPS key requirements are satisfied, false if not.
   */
  virtual bool checkFips() PURE;

  /**
   * Check whether the shared key method is available.
   * @return true if the shared key method is available, false if not.
   */
  virtual bool isAvailable() PURE;

#ifdef OPENSSL_IS_BORINGSSL
  /**
   * Get the shared key methods from the provider.
   * @return the shared key methods associated with this provider and
   * configuration.
   */
  virtual BoringSslSharedKeyMethodSharedPtr getBoringSslSharedKeyMethod() PURE;
#endif
};

using SharedKeyMethodProviderSharedPtr = std::shared_ptr<SharedKeyMethodProvider>;

class SharedKeyMethodProviderFactory : public Config::TypedFactory {
public:
  /**
   * Creates an shared key method provider from the provided config.
   */
  virtual SharedKeyMethodProviderSharedPtr createSharedKeyMethodProvider(
      const Protobuf::Message& config,
      Server::Configuration::ServerFactoryContext& server_factory_context) PURE;

  std::string category() const override { return "envoy.tls.shared_key_providers"; }
};

} // namespace Ssl
} // namespace Envoy
