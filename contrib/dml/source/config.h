#pragma once

#include "source/common/buffer/memory_interface.h"

namespace Envoy {
namespace Extensions {
namespace Buffer {
namespace Dml {

class MemoryInterfaceExtension : public Envoy::Buffer::MemoryInterfaceExtension {
public:
  MemoryInterfaceExtension(Envoy::Buffer::MemoryInterface& memory_interface)
      : Envoy::Buffer::MemoryInterfaceExtension(memory_interface) {}
};

class MemoryInterface : public Envoy::Buffer::MemoryInterfaceBase {
public:
  MemoryInterface();
  MemoryInterface(uint32_t software_cutoff, uint32_t hardware_cutoff);

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

DECLARE_FACTORY(MemoryInterface);

} // namespace Dml
} // namespace Buffer
} // namespace Extensions
} // namespace Envoy
