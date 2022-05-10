#include "contrib/dml/source/config.h"

#include "contrib/envoy/extensions/dml/v3alpha/dml_memory_interface.pb.h"
#include "contrib/envoy/extensions/dml/v3alpha/dml_memory_interface.pb.validate.h"

#ifndef DML_DISABLED
#include "dml/dml.hpp"
#endif

namespace Envoy {
namespace Extensions {
namespace Buffer {
namespace Dml {

MemoryInterface::MemoryInterface() : MemoryInterface(0, 0) {}

MemoryInterface::MemoryInterface(uint32_t software_cutoff, uint32_t hardware_cutoff)
    : software_cutoff_(software_cutoff), hardware_cutoff_(hardware_cutoff) {}

void MemoryInterface::memoryCopy(void* dest, const void* src, size_t n) {
#ifndef DML_DISABLED
  if (!n) {
    return;
  }

  // Hardware offloading.
  dml::const_data_view src_view = dml::make_view(src, n);
  dml::data_view dest_view = dml::make_view(dest, n);
  if (n >= hardware_cutoff_) {
    const dml::mem_copy_result result =
        dml::execute<dml::hardware>(dml::mem_copy, src_view, dest_view);
    const dml::status_code code = result.status;
    if (code == dml::status_code::ok) {
      ENVOY_LOG_MISC(trace, "DSA copy memory size {}", n);
      return;
    }

    ENVOY_LOG_MISC(warn, "DSA failed to copy memory with errno {}", static_cast<uint32_t>(code));
  }

  // DML-optimized memory copy.
  if (n >= software_cutoff_) {
    const dml::mem_copy_result result =
        dml::execute<dml::software>(dml::mem_copy, src_view, dest_view);
    const dml::status_code code = result.status;
    ASSERT(code == dml::status_code::ok,
           fmt::format("DML failed to copy memory with errno {}", static_cast<uint32_t>(code)));
    ENVOY_LOG_MISC(trace, "DML copy memory size {}", n);
    return;
  }
#endif

  // Fallback to default memory copy.
  memcpy(dest, src, n);
}

void MemoryInterface::batchMemoryCopy(const std::vector<void*>& dests,
                                      const std::vector<const void*>& srcs,
                                      const std::vector<size_t>& ns) {
  ASSERT(dests.size() == srcs.size());
  ASSERT(srcs.size() == ns.size());

  const size_t size = srcs.size();
  if (!size) {
    return;
  }
#ifndef DML_DISABLED
  using Handler = dml::handler<dml::mem_copy_operation, std::allocator<unsigned char>>;

  std::vector<Handler> hardware_handlers;
  hardware_handlers.reserve(size);
  std::vector<size_t> hardware_ops;
  hardware_ops.reserve(size);
  std::vector<size_t> software_ops;
  software_ops.reserve(size);

  // Submit DSA-met memory copy.
  for (size_t i = 0; i < size; i++) {
    const size_t n = ns[i];
    if (!n) {
      continue;
    }

    if (n >= hardware_cutoff_) {
      dml::const_data_view src_view = dml::make_view(srcs[i], n);
      dml::data_view dest_view = dml::make_view(dests[i], n);
      hardware_handlers.push_back(dml::submit<dml::hardware>(dml::mem_copy, src_view, dest_view));
      hardware_ops.push_back(i);
      continue;
    }

    software_ops.push_back(i);
  }

  // While DSA is copying memory, perform DSA-unmet memory copy in CPU.
  for (const size_t i : software_ops) {
    memoryCopy(dests[i], srcs[i], ns[i]);
  }
  software_ops.clear();

  // Check and fall failed to software path.
  const size_t hardware_ops_size = hardware_ops.size();
  for (size_t i = 0; i < hardware_ops_size; i++) {
    const dml::status_code code = hardware_handlers[i].get().status;
    if (code == dml::status_code::ok) {
      ENVOY_LOG_MISC(trace, "DSA copy memory");
      continue;
    }

    ENVOY_LOG_MISC(warn, "DSA failed to copy memory with errno {}", static_cast<uint32_t>(code));
    software_ops.push_back(hardware_ops[i]);
  }
  for (const size_t i : software_ops) {
    memoryCopy(dests[i], srcs[i], ns[i]);
  }
#else
  for (size_t i = 0; i < size; i++) {
    memoryCopy(dests[i], srcs[i], ns[i]);
  }
#endif
}

Server::BootstrapExtensionPtr
MemoryInterface::createBootstrapExtension(const Protobuf::Message& config,
                                          Server::Configuration::ServerFactoryContext& context) {
  const auto dml_config =
      MessageUtil::downcastAndValidate<const envoy::extensions::dml::v3alpha::DmlMemoryInterface&>(
          config, context.messageValidationVisitor());

  software_cutoff_ = dml_config.software_cutoff();
  hardware_cutoff_ = dml_config.hardware_cutoff();

#ifdef DML_DISABLED
  throw EnvoyException("X86_64 architecture and gcc compiler is required for DML.");
#else
  return std::make_unique<MemoryInterfaceExtension>(*this);
#endif
}

ProtobufTypes::MessagePtr MemoryInterface::createEmptyConfigProto() {
  return std::make_unique<envoy::extensions::dml::v3alpha::DmlMemoryInterface>();
}

REGISTER_FACTORY(MemoryInterface, Server::Configuration::BootstrapExtensionFactory);

} // namespace Dml
} // namespace Buffer
} // namespace Extensions
} // namespace Envoy
