#pragma once

#include "envoy/extensions/transport_sockets/tls/v3/cert.pb.h"
#include "envoy/ssl/shared_key/shared_key.h"
#include "envoy/ssl/shared_key/shared_key_config.h"

namespace Envoy {
namespace Extensions {
namespace SharedKeyMethodProvider {
namespace CryptoMb {

class CryptoMbSharedKeyMethodFactory : public Ssl::SharedKeyMethodProviderInstanceFactory,
                                       public Logger::Loggable<Logger::Id::connection> {
public:
  // Ssl::SharedKeyMethodProviderInstanceFactory
  Ssl::SharedKeyMethodProviderSharedPtr createSharedKeyMethodProviderInstance(
      const envoy::extensions::transport_sockets::tls::v3::SharedKeyProvider& message,
      Server::Configuration::TransportSocketFactoryContext& shared_key_provider_context) override;
  std::string name() const override { return "cryptomb"; };
};

} // namespace CryptoMb
} // namespace SharedKeyMethodProvider
} // namespace Extensions
} // namespace Envoy
