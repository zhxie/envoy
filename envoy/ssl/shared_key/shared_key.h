#pragma once

#include <functional>
#include <string>

#include "envoy/common/pure.h"
#include "envoy/event/dispatcher.h"
#include "envoy/extensions/transport_sockets/tls/v3/cert.pb.h"
#include "envoy/ssl/shared_key/shared_key_callbacks.h"

#include "openssl/ssl.h"

namespace Envoy {
namespace Server {
namespace Configuration {
// Prevent a dependency loop with the forward declaration.
class TransportSocketFactoryContext;
} // namespace Configuration
} // namespace Server

namespace Ssl {

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

/**
 * A manager for finding correct user-provided functions for handling BoringSSL shared key
 * operations.
 */
class SharedKeyMethodManager {
public:
  virtual ~SharedKeyMethodManager() = default;

  /**
   * Finds and returns a shared key operations provider for BoringSSL.
   *
   * @param config a protobuf message object containing a SharedKeyProvider message.
   * @param factory_context context that provides components for creating and
   * initializing connections using asynchronous shared key operations.
   * @return SharedKeyMethodProvider the shared key operations provider, or nullptr if
   * no provider can be used with the context configuration.
   */
  virtual SharedKeyMethodProviderSharedPtr createSharedKeyMethodProvider(
      const envoy::extensions::transport_sockets::tls::v3::SharedKeyProvider& config,
      Envoy::Server::Configuration::TransportSocketFactoryContext& factory_context) PURE;
};

} // namespace Ssl
} // namespace Envoy
