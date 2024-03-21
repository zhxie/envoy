#pragma once

#include "envoy/ssl/shared_key_provider.h"

namespace Envoy {
namespace Server {

Ssl::SharedKeyMethodProviderSharedPtr
createSharedKeyMethodProvider(const envoy::config::bootstrap::v3::Bootstrap& bootstrap,
                              ProtobufMessage::ValidationVisitor& validation_visitor,
                              Configuration::ServerFactoryContext& server_factory_context);

} // namespace Server
} // namespace Envoy
