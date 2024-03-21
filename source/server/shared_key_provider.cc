#include "source/server/shared_key_provider.h"

#include "source/common/config/utility.h"
#include "source/extensions/transport_sockets/tls/context_impl.h"

namespace Envoy {
namespace Server {

Ssl::SharedKeyMethodProviderSharedPtr
createSharedKeyMethodProvider(const envoy::config::bootstrap::v3::Bootstrap& bootstrap,
                              ProtobufMessage::ValidationVisitor& validation_visitor,
                              Configuration::ServerFactoryContext& server_factory_context) {
  Ssl::SharedKeyMethodProviderSharedPtr provider;
  if (bootstrap.has_shared_key_provider()) {
    const auto& shared_key_provider = bootstrap.shared_key_provider();
    Ssl::SharedKeyMethodProviderFactory& factory =
        Config::Utility::getAndCheckFactory<Ssl::SharedKeyMethodProviderFactory>(
            shared_key_provider);
    auto config = Config::Utility::translateAnyToFactoryConfig(shared_key_provider.typed_config(),
                                                               validation_visitor, factory);
    provider = factory.createSharedKeyMethodProvider(*config, server_factory_context);
  }
  Extensions::TransportSockets::Tls::SharedKeyMethodProviderSingleton::clear();
  if (provider) {
    Extensions::TransportSockets::Tls::SharedKeyMethodProviderSingleton::initialize(provider.get());
  }

  return provider;
}

} // namespace Server
} // namespace Envoy
