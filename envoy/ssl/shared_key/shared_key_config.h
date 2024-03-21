#pragma once

#include "envoy/config/typed_config.h"
#include "envoy/extensions/transport_sockets/tls/v3/cert.pb.h"
#include "envoy/registry/registry.h"
#include "envoy/ssl/shared_key/shared_key.h"

namespace Envoy {
namespace Ssl {

// Base class which the shared key operation provider implementations can register.

class SharedKeyMethodProviderInstanceFactory : public Config::UntypedFactory {
public:
  ~SharedKeyMethodProviderInstanceFactory() override = default;

  /**
   * Create a particular SharedKeyMethodProvider implementation. If the implementation is
   * unable to produce a SharedKeyMethodProvider with the provided parameters, it should throw
   * an EnvoyException. The returned pointer should always be valid.
   * @param config supplies the custom proto configuration for the SharedKeyMethodProvider
   * @param context supplies the factory context
   */
  virtual SharedKeyMethodProviderSharedPtr createSharedKeyMethodProviderInstance(
      const envoy::extensions::transport_sockets::tls::v3::SharedKeyProvider& config,
      Server::Configuration::TransportSocketFactoryContext& factory_context) PURE;

  std::string category() const override { return "envoy.tls.shared_key_providers"; };
};

} // namespace Ssl
} // namespace Envoy
