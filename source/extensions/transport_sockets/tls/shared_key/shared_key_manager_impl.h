#pragma once

#include "envoy/extensions/transport_sockets/tls/v3/cert.pb.h"
#include "envoy/ssl/shared_key/shared_key.h"
#include "envoy/ssl/shared_key/shared_key_config.h"

namespace Envoy {
namespace Extensions {
namespace TransportSockets {
namespace Tls {

class SharedKeyMethodManagerImpl : public virtual Ssl::SharedKeyMethodManager {
public:
  // Ssl::SharedKeyMethodManager
  Ssl::SharedKeyMethodProviderSharedPtr createSharedKeyMethodProvider(
      const envoy::extensions::transport_sockets::tls::v3::SharedKeyProvider& config,
      Server::Configuration::TransportSocketFactoryContext& factory_context) override;
};

} // namespace Tls
} // namespace TransportSockets
} // namespace Extensions
} // namespace Envoy
