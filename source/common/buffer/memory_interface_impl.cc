#include "source/common/buffer/memory_interface_impl.h"

#include <cstring>

#include "envoy/extensions/buffer/memory_interface/v3/default_memory_interface.pb.h"

namespace Envoy {
namespace Buffer {

void MemoryInterfaceImpl::memoryCopy(void* dest, const void* src, size_t n) const {
  memcpy(dest, src, n);
}

void MemoryInterfaceImpl::batchMemoryCopy(const std::vector<void*>& dests,
                                          const std::vector<const void*>& srcs,
                                          const std::vector<size_t>& ns) const {
  ASSERT(dests.size() == srcs.size());
  ASSERT(srcs.size() == ns.size());
  for (size_t i = 0; i < srcs.size(); i++) {
    memoryCopy(dests[i], srcs[i], ns[i]);
  }
}

Server::BootstrapExtensionPtr
MemoryInterfaceImpl::createBootstrapExtension(const Protobuf::Message&,
                                              Server::Configuration::ServerFactoryContext&) {
  return std::make_unique<MemoryInterfaceExtension>(*this);
}

ProtobufTypes::MessagePtr MemoryInterfaceImpl::createEmptyConfigProto() {
  return std::make_unique<
      envoy::extensions::buffer::memory_interface::v3::DefaultMemoryInterface>();
}

REGISTER_FACTORY(MemoryInterfaceImpl, Server::Configuration::BootstrapExtensionFactory);

static MemoryInterfaceLoader* memory_interface_ =
    new MemoryInterfaceLoader(std::make_unique<MemoryInterfaceImpl>());

} // namespace Buffer
} // namespace Envoy
