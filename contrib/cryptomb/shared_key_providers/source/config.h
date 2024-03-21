#pragma once

#include "envoy/extensions/transport_sockets/tls/v3/cert.pb.h"
#include "envoy/ssl/shared_key_provider.h"

#include "contrib/envoy/extensions/shared_key_providers/cryptomb/v3alpha/cryptomb.pb.h"
#include "contrib/envoy/extensions/shared_key_providers/cryptomb/v3alpha/cryptomb.pb.validate.h"

namespace Envoy {
namespace Extensions {
namespace SharedKeyMethodProvider {
namespace CryptoMb {

class CryptoMbSharedKeyMethodFactory : public Ssl::SharedKeyMethodProviderFactory,
                                       public Logger::Loggable<Logger::Id::connection> {
public:
  // Ssl::SharedKeyMethodProviderFactory
  Ssl::SharedKeyMethodProviderSharedPtr createSharedKeyMethodProvider(
      const Protobuf::Message& config,
      Server::Configuration::ServerFactoryContext& server_factory_context) override;
  ProtobufTypes::MessagePtr createEmptyConfigProto() override {
    return std::make_unique<envoy::extensions::shared_key_providers::cryptomb::v3alpha::
                                CryptoMbSharedKeyMethodConfig>();
  }
  std::string name() const override { return "cryptomb"; };
};

} // namespace CryptoMb
} // namespace SharedKeyMethodProvider
} // namespace Extensions
} // namespace Envoy
