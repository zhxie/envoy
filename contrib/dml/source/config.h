#pragma once

#include "source/common/buffer/memory_interface.h"

namespace Envoy {
namespace Extensions {
namespace Buffer {
namespace Dml {

class DmlMemoryInterfaceExtension : public Envoy::Buffer::MemoryInterfaceExtension {
public:
  DmlMemoryInterfaceExtension(Envoy::Buffer::MemoryInterface& memory_interface)
      : Envoy::Buffer::MemoryInterfaceExtension(memory_interface) {}
};

class DmlMemoryInterface : public Envoy::Buffer::MemoryInterfaceBase {
public:
  // Buffer::MemoryInterface
  void memoryCopy(void* dest, const void* src, size_t n) const override;

  // Server::Configuration::BootstrapExtensionFactory
  Server::BootstrapExtensionPtr
  createBootstrapExtension(const Protobuf::Message& config,
                           Server::Configuration::ServerFactoryContext& context) override;
  ProtobufTypes::MessagePtr createEmptyConfigProto() override;
  std::string name() const override { return "envoy.extensions.dml.dml_memory_interface"; };

private:
  uint32_t software_cutoff_;
  uint32_t hardware_cutoff_;
};

DECLARE_FACTORY(DmlMemoryInterface);

} // namespace Dml
} // namespace Buffer
} // namespace Extensions
} // namespace Envoy
