#include "contrib/dml/source/config.h"

#include "contrib/envoy/extensions/dml/v3alpha/dml_memory_interface.pb.h"
#include "contrib/envoy/extensions/dml/v3alpha/dml_memory_interface.pb.validate.h"

#ifndef DML_DISABLED
#include <dml/dml.hpp>
#endif

namespace Envoy {
namespace Extensions {
namespace Buffer {
namespace Dml {

void DmlMemoryInterface::memoryCopy(void* dest, const void* src, size_t n) const {
  if (!n) {
    return;
  }

#ifndef DML_DISABLED
  dml::const_data_view src_view = dml::make_view(src, n);
  dml::data_view dest_view = dml::make_view(dest, n);
  if (n >= hardware_cutoff_) {
    const dml::mem_copy_result result =
        dml::execute<dml::hardware>(dml::mem_copy, src_view, dest_view);
    const dml::status_code code = result.status;
    if (code == dml::status_code::ok) {
      return;
    } else {
      ENVOY_LOG_MISC(warn, "DSA failed to copy memory with errno {}", static_cast<uint32_t>(code));
    }
  }

  if (n >= software_cutoff_) {
    const dml::mem_copy_result result =
        dml::execute<dml::software>(dml::mem_copy, src_view, dest_view);
    const dml::status_code code = result.status;
    ASSERT(code == dml::status_code::ok,
           fmt::format("DML failed to copy memory with errno {}", static_cast<uint32_t>(code)));
  }
#endif

  memcpy(dest, src, n);
}

Server::BootstrapExtensionPtr
DmlMemoryInterface::createBootstrapExtension(const Protobuf::Message& config,
                                             Server::Configuration::ServerFactoryContext& context) {
  const auto dml_config =
      MessageUtil::downcastAndValidate<const envoy::extensions::dml::v3alpha::DmlMemoryInterface&>(
          config, context.messageValidationVisitor());

  software_cutoff_ = dml_config.software_cutoff();
  hardware_cutoff_ = dml_config.hardware_cutoff();

#ifdef DML_DISABLED
  throw EnvoyException("X86_64 architecture and gcc compiler is required for DML.");
#else
  return std::make_unique<DmlMemoryInterfaceExtension>(*this);
#endif
}

ProtobufTypes::MessagePtr DmlMemoryInterface::createEmptyConfigProto() {
  return std::make_unique<envoy::extensions::dml::v3alpha::DmlMemoryInterface>();
}

REGISTER_FACTORY(DmlMemoryInterface, Server::Configuration::BootstrapExtensionFactory);

} // namespace Dml
} // namespace Buffer
} // namespace Extensions
} // namespace Envoy
