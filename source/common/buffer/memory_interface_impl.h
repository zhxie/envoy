#pragma once

#include "source/common/buffer/memory_interface.h"

namespace Envoy {
namespace Buffer {

class MemoryInterfaceImpl : public MemoryInterfaceBase {
public:
  // MemoryInterface
  void memoryCopy(void* dest, const void* src, size_t n) const override;
  void batchMemoryCopy(const std::vector<void*>& dests, const std::vector<const void*>& srcs,
                       const std::vector<size_t>& ns) const override;

  // Server::Configuration::BootstrapExtensionFactory
  Server::BootstrapExtensionPtr
  createBootstrapExtension(const Protobuf::Message& config,
                           Server::Configuration::ServerFactoryContext& context) override;

  ProtobufTypes::MessagePtr createEmptyConfigProto() override;
  std::string name() const override {
    return "envoy.extensions.buffer.memory_interface.default_memory_interface";
  };
};

DECLARE_FACTORY(MemoryInterfaceImpl);

} // namespace Buffer
} // namespace Envoy
