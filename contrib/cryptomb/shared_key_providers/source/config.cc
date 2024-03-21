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

namespace Envoy {
namespace Extensions {
namespace SharedKeyMethodProvider {
namespace CryptoMb {

Ssl::SharedKeyMethodProviderSharedPtr CryptoMbSharedKeyMethodFactory::createSharedKeyMethodProvider(
    const Protobuf::Message& config,
    Server::Configuration::ServerFactoryContext& server_factory_context) {
  const auto cryptomb =
      MessageUtil::downcastAndValidate<const envoy::extensions::shared_key_providers::cryptomb::
                                           v3alpha::CryptoMbSharedKeyMethodConfig&>(
          config, server_factory_context.messageValidationVisitor());
#ifdef IPP_CRYPTO_DISABLED
  throw EnvoyException("X86_64 architecture is required for cryptomb provider.");
#else
  std::chrono::milliseconds poll_delay =
      std::chrono::milliseconds(PROTOBUF_GET_MS_OR_DEFAULT(cryptomb, poll_delay, 200));
  PrivateKeyMethodProvider::CryptoMb::IppCryptoSharedPtr ipp =
      std::make_shared<PrivateKeyMethodProvider::CryptoMb::IppCryptoImpl>();
  Ssl::SharedKeyMethodProviderSharedPtr provider =
      std::make_shared<CryptoMbSharedKeyMethodProvider>(
          server_factory_context.threadLocal(), server_factory_context.scope(), ipp, poll_delay);
  return provider;
#endif
}

REGISTER_FACTORY(CryptoMbSharedKeyMethodFactory, Ssl::SharedKeyMethodProviderFactory);

} // namespace CryptoMb
} // namespace SharedKeyMethodProvider
} // namespace Extensions
} // namespace Envoy
