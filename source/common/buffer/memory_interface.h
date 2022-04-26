#pragma once

#include "envoy/buffer/memory_interface.h"
#include "envoy/registry/registry.h"
#include "envoy/server/bootstrap_extension_config.h"

#include "source/common/singleton/threadsafe_singleton.h"

namespace Envoy {
namespace Buffer {

// Wrapper for MemoryInterface instances returned by createBootstrapExtension() which must be
// implemented by all factories that derive MemoryInterfaceBase.
class MemoryInterfaceExtension : public Server::BootstrapExtension {
public:
  MemoryInterfaceExtension(MemoryInterface& memory_interface)
      : memory_interface_(memory_interface) {}

  // Server::BootstrapExtension
  void onServerInitialized() override {}

protected:
  MemoryInterface& memory_interface_;
};

// Class to be derived by all MemoryInterface implementations.
class MemoryInterfaceBase : public MemoryInterface,
                            public Server::Configuration::BootstrapExtensionFactory {};

/**
 * Lookup MemoryInterface instance by name
 * @param name Name of the memory interface to be looked up
 * @return Pointer to @ref MemoryInterface instance that registered using the name of nullptr
 */
static inline const MemoryInterface* memoryInterface(std::string name) {
  auto factory =
      Registry::FactoryRegistry<Server::Configuration::BootstrapExtensionFactory>::getFactory(name);
  return dynamic_cast<MemoryInterface*>(factory);
}

using MemoryInterfaceSingleton = InjectableSingleton<MemoryInterface>;
using MemoryInterfaceLoader = ScopedInjectableLoader<MemoryInterface>;

} // namespace Buffer
} // namespace Envoy
