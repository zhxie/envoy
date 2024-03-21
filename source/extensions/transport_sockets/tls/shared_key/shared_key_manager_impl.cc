#include "source/extensions/transport_sockets/tls/shared_key/shared_key_manager_impl.h"

#include "envoy/extensions/transport_sockets/tls/v3/cert.pb.h"
#include "envoy/registry/registry.h"

namespace Envoy {
namespace Extensions {
namespace TransportSockets {
namespace Tls {

Envoy::Ssl::SharedKeyMethodProviderSharedPtr
SharedKeyMethodManagerImpl::createSharedKeyMethodProvider(
    const envoy::extensions::transport_sockets::tls::v3::SharedKeyProvider& config,
    Server::Configuration::TransportSocketFactoryContext& factory_context) {

  Ssl::SharedKeyMethodProviderInstanceFactory* factory =
      Registry::FactoryRegistry<Ssl::SharedKeyMethodProviderInstanceFactory>::getFactory(
          config.provider_name());

  // Create a new provider instance with the configuration.
  if (factory) {
    return factory->createSharedKeyMethodProviderInstance(config, factory_context);
  }

  return nullptr;
}

} // namespace Tls
} // namespace TransportSockets
} // namespace Extensions
} // namespace Envoy
