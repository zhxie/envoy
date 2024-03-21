#include "contrib/cryptomb/shared_key_providers/source/config.h"

#include <memory>

#include "envoy/registry/registry.h"
#include "envoy/server/transport_socket_config.h"

#include "source/common/common/logger.h"
#include "source/common/config/utility.h"
#include "source/common/protobuf/message_validator_impl.h"
#include "source/common/protobuf/utility.h"

#ifndef IPP_CRYPTO_DISABLED
#include "contrib/cryptomb/private_key_providers/source/ipp_crypto_impl.h"
#include "contrib/cryptomb/shared_key_providers/source/cryptomb_shared_key_provider.h"
#endif

#include "contrib/envoy/extensions/shared_key_providers/cryptomb/v3alpha/cryptomb.pb.h"
#include "contrib/envoy/extensions/shared_key_providers/cryptomb/v3alpha/cryptomb.pb.validate.h"

namespace Envoy {
namespace Extensions {
namespace SharedKeyMethodProvider {
namespace CryptoMb {

Ssl::SharedKeyMethodProviderSharedPtr
CryptoMbSharedKeyMethodFactory::createSharedKeyMethodProviderInstance(
    const envoy::extensions::transport_sockets::tls::v3::SharedKeyProvider& proto_config,
    Server::Configuration::TransportSocketFactoryContext& shared_key_provider_context) {
  ProtobufTypes::MessagePtr message = std::make_unique<
      envoy::extensions::shared_key_providers::cryptomb::v3alpha::CryptoMbSharedKeyMethodConfig>();

  Config::Utility::translateOpaqueConfig(proto_config.typed_config(),
                                         ProtobufMessage::getNullValidationVisitor(), *message);
  const envoy::extensions::shared_key_providers::cryptomb::v3alpha::CryptoMbSharedKeyMethodConfig
      conf =
          MessageUtil::downcastAndValidate<const envoy::extensions::shared_key_providers::cryptomb::
                                               v3alpha::CryptoMbSharedKeyMethodConfig&>(
              *message, shared_key_provider_context.messageValidationVisitor());
  Ssl::SharedKeyMethodProviderSharedPtr provider = nullptr;
#ifdef IPP_CRYPTO_DISABLED
  throw EnvoyException("X86_64 architecture is required for cryptomb provider.");
#else
  PrivateKeyMethodProvider::CryptoMb::IppCryptoSharedPtr ipp =
      std::make_shared<PrivateKeyMethodProvider::CryptoMb::IppCryptoImpl>();
  provider =
      std::make_shared<CryptoMbSharedKeyMethodProvider>(conf, shared_key_provider_context, ipp);
#endif
  return provider;
}

REGISTER_FACTORY(CryptoMbSharedKeyMethodFactory, Ssl::SharedKeyMethodProviderInstanceFactory);

} // namespace CryptoMb
} // namespace SharedKeyMethodProvider
} // namespace Extensions
} // namespace Envoy
