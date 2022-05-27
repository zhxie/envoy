#include "contrib/idxd/source/config.h"

#include "contrib/envoy/extensions/idxd/v3alpha/idxd_memory_interface.pb.h"
#include "contrib/envoy/extensions/idxd/v3alpha/idxd_memory_interface.pb.validate.h"

#ifndef IDXD_DISABLED
#include "x86intrin.h"
#endif

namespace Envoy {
namespace Extensions {
namespace Buffer {
namespace Idxd {

MemoryInterface::MemoryInterface() = default;

MemoryInterface::MemoryInterface(const std::string& device, const std::string& name,
                                 uint32_t cutoff, uint64_t wait_cycle)
    : cutoff_(cutoff), wait_cycle_(wait_cycle) {
#ifndef IDXD_DISABLED
  initialize(device, name);
#endif
}

MemoryInterface::~MemoryInterface() {
#ifndef IDXD_DISABLED
  if (portal_) {
    munmap(portal_, 0x1000);
  }
#endif
}

void MemoryInterface::memoryCopy(void* dest, const void* src, size_t n) {
#ifndef IDXD_DISABLED
  if (!n) {
    return;
  }

  if (n >= cutoff_) {
    // Prepare completion record.
    completion_record* record = createCompletionRecord();

    // Submit descriptor.
    hw_desc* descriptor =
        createDescriptor(reinterpret_cast<uint64_t>(record), reinterpret_cast<uint64_t>(dest),
                         reinterpret_cast<uint64_t>(src), n);
    uint8_t status = submit(descriptor);
    if (status) {
      // Wait completion record until completed.
      wait(record);
      status = record->status;
      free(descriptor);
      free(record);
      if (status == DSA_COMP_SUCCESS) {
        ENVOY_LOG_MISC(trace, "DSA copy memory size {}", n);
        return;
      }

      ENVOY_LOG_MISC(warn, "DSA failed to memory copy with errno {}", status);
    } else {
      free(descriptor);
      free(record);
      ENVOY_LOG_MISC(warn, "IDXD failed to submit descriptor with errno {}", status);
    }
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

#ifndef IDXD_DISABLED
  std::vector<completion_record*> completion_records;
  completion_records.reserve(size);
  std::vector<hw_desc*> descriptors;
  descriptors.reserve(size);
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

    if (n >= cutoff_) {
      // Prepare completion record.
      completion_record* record = createCompletionRecord();

      // Submit descriptor.
      hw_desc* descriptor =
          createDescriptor(reinterpret_cast<uint64_t>(record), reinterpret_cast<uint64_t>(dests[i]),
                           reinterpret_cast<uint64_t>(srcs[i]), n);
      uint8_t status = submit(descriptor);
      if (status) {
        completion_records.push_back(record);
        descriptors.push_back(descriptor);
        hardware_ops.push_back(i);
        continue;
      }

      free(descriptor);
      free(record);
      ENVOY_LOG_MISC(warn, "IDXD failed to submit descriptor with errno {}", status);
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
    // Wait completion record until completed.
    completion_record* record = completion_records[i];
    hw_desc* descriptor = descriptors[i];
    wait(record);
    uint8_t status = record->status;
    uint32_t size = descriptor->xfer_size;
    free(descriptor);
    free(record);
    if (status == DSA_COMP_SUCCESS) {
      ENVOY_LOG_MISC(trace, "DSA copy memory size {}", size);
      continue;
    }

    ENVOY_LOG_MISC(warn, "DSA failed to memory copy with errno {}", status);
    software_ops.push_back(i);
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

#ifndef IDXD_DISABLED
completion_record* MemoryInterface::createCompletionRecord() const {
  completion_record* record = reinterpret_cast<completion_record*>(
      aligned_alloc(completion_size_, sizeof(struct completion_record)));
  record->status = 0;
  return record;
}

hw_desc* MemoryInterface::createDescriptor(uint64_t record, uint64_t dest, uint64_t src,
                                           size_t n) const {
  ASSERT(record % completion_size_ == 0);

  hw_desc* descriptor = reinterpret_cast<hw_desc*>(malloc(sizeof(struct hw_desc)));
  memset(descriptor, 0, sizeof(struct hw_desc));

  // Request completion and completion address valid.
  descriptor->flags = IDXD_OP_FLAG_RCR | IDXD_OP_FLAG_CRAV;
  if (block_on_fault_) {
    descriptor->flags |= IDXD_OP_FLAG_BOF;
  }
  descriptor->opcode = DSA_OPCODE_MEMMOVE;
  descriptor->completion_addr = record;
  descriptor->src_addr = src;
  descriptor->dst_addr = dest;
  descriptor->xfer_size = n;

  return descriptor;
}

accfg_wq* MemoryInterface::getWorkQueueByDevice(const std::string& device) const {
  for (accfg_device* dev = accfg_device_get_first(context_); dev != nullptr;
       dev = accfg_device_get_next(dev)) {
    for (accfg_wq* work_queue = accfg_wq_get_first(dev); work_queue != nullptr;
         work_queue = accfg_wq_get_next(work_queue)) {
      if (device == accfg_wq_get_devname(work_queue)) {
        return work_queue;
      }
    }
  }

  return nullptr;
}

accfg_wq* MemoryInterface::getWorkQueueByName(const std::string& name) const {
  for (accfg_device* dev = accfg_device_get_first(context_); dev != nullptr;
       dev = accfg_device_get_next(dev)) {
    for (accfg_wq* work_queue = accfg_wq_get_first(dev); work_queue != nullptr;
         work_queue = accfg_wq_get_next(work_queue)) {
      if (name == accfg_wq_get_type_name(work_queue)) {
        return work_queue;
      }
    }
  }

  return nullptr;
}

size_t MemoryInterface::getWorkQueueSize() const { return accfg_wq_get_size(work_queue_); }

void MemoryInterface::initialize(const std::string& device, const std::string& name) {
  int result = accfg_new(&context_);
  if (result != 0) {
    throw EnvoyException(
        fmt::format("Failed to instantiate accel-config contex with errno {}.", result));
  }

  // Get work queue.
  if (!device.empty()) {
    work_queue_ = getWorkQueueByDevice(device);
    if (!work_queue_) {
      throw EnvoyException(fmt::format("Didn't find a work queue with device '{}'.", device));
    }
  } else {
    work_queue_ = getWorkQueueByName(name);
    if (!work_queue_) {
      throw EnvoyException(fmt::format("Didn't find a work queue with name '{}'.", name));
    }
    std::string work_queue_device = accfg_wq_get_devname(work_queue_);
    ENVOY_LOG_MISC(debug, "Work queue '{}' is chosen", work_queue_device);
  }

  // Check work queue.
  if (accfg_wq_get_state(work_queue_) != accfg_wq_state::ACCFG_WQ_ENABLED) {
    throw EnvoyException(fmt::format("State of work queue is not 'enabled'."));
  }
  if (accfg_wq_get_type(work_queue_) != accfg_wq_type::ACCFG_WQT_USER) {
    throw EnvoyException(fmt::format("Type of work queue is not 'user'."));
  }

  // Get block on fault.
  block_on_fault_ = accfg_wq_get_block_on_fault(work_queue_);
  if (block_on_fault_) {
    ENVOY_LOG_MISC(debug, "Work queue will block on fault");
  }

  // Get device and its completion size.
  device_ = accfg_wq_get_device(work_queue_);
  completion_size_ = accfg_device_get_compl_size(device_);

  // Get work queue mode.
  mode_ = accfg_wq_get_mode(work_queue_);
  switch (mode_) {
  case accfg_wq_mode::ACCFG_WQ_SHARED:
    ENVOY_LOG_MISC(debug, "Work queue is enabled in shared mode");
    break;
  case accfg_wq_mode::ACCFG_WQ_DEDICATED:
    ENVOY_LOG_MISC(debug, "Work queue is enabled in dedicated mode");
    break;
  case accfg_wq_mode::ACCFG_WQ_MODE_UNKNOWN:
    throw EnvoyException(fmt::format("Unexpected mode of work queue."));
  }

  // Map user device.
  char path[256];
  result = accfg_wq_get_user_dev_path(work_queue_, path, sizeof(path));
  if (result != 0) {
    throw EnvoyException(fmt::format("Failed to get user device path with errno {}.", result));
  }
  int fd = open(path, O_RDWR);
  if (fd < 0) {
    throw EnvoyException(
        fmt::format("Failed to open user device path {} with errno {}.", std::string(path), fd));
  }
  portal_ = mmap(nullptr, 0x1000, PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
  close(fd);
  if (portal_ == MAP_FAILED) {
    throw EnvoyException(fmt::format("Failed to map user device path {}.", std::string(path)));
  }
}

uint8_t MemoryInterface::submit(const hw_desc* descriptor) {
  switch (mode_) {
  case accfg_wq_mode::ACCFG_WQ_SHARED: {
    // ENQCMD.
    uint8_t status = 0;
    asm volatile("sfence\n"
                 ".byte 0xf2, 0x0f, 0x38, 0xf8, 0x02\n"
                 "setnz %0\n"
                 : "=r"(status)
                 : "a"(portal_), "d"(descriptor));
    return status;
  }
  case accfg_wq_mode::ACCFG_WQ_DEDICATED:
    // MOVDIR64B.
    asm volatile("sfence\n"
                 ".byte 0x66, 0x0f, 0x38, 0xf8, 0x02\n"
                 :
                 : "a"(portal_), "d"(descriptor));
    return 1;
  case accfg_wq_mode::ACCFG_WQ_MODE_UNKNOWN:
    throw EnvoyException(fmt::format("Unexpected mode of work queue."));
  }
  PANIC_DUE_TO_CORRUPT_ENUM;
}

void MemoryInterface::wait(completion_record* record) const {
  volatile uint8_t* status = &record->status;
  uint64_t deadline = __rdtsc() + wait_cycle_;
  while (*status == 0) {
    // UMONITOR.
    asm volatile(".byte 0xf3, 0x48, 0x0f, 0xae, 0xf0" : : "a"(status));
    // UMWAIT.
    asm volatile(".byte 0xf2, 0x48, 0x0f, 0xae, 0xf1"
                 :
                 : "c"(0), "a"(uint32_t(deadline)), "d"(uint32_t(deadline >> 32)));
  }
}
#endif

Server::BootstrapExtensionPtr
MemoryInterface::createBootstrapExtension(const Protobuf::Message& config,
                                          Server::Configuration::ServerFactoryContext& context) {
  const auto idxd_config = MessageUtil::downcastAndValidate<
      const envoy::extensions::idxd::v3alpha::IdxdMemoryInterface&>(
      config, context.messageValidationVisitor());

  const auto& device = idxd_config.device();
  const auto& name = idxd_config.name();
  cutoff_ = idxd_config.cutoff();
  wait_cycle_ = idxd_config.wait_cycle();
  if (!wait_cycle_) {
    wait_cycle_ = 200;
  }

#ifdef IDXD_DISABLED
  throw EnvoyException("X86_64 architecture is required for DML.");
#else
  initialize(device, name);

  return std::make_unique<Envoy::Buffer::MemoryInterfaceExtension>(*this);
#endif
}

ProtobufTypes::MessagePtr MemoryInterface::createEmptyConfigProto() {
  return std::make_unique<envoy::extensions::idxd::v3alpha::IdxdMemoryInterface>();
}

REGISTER_FACTORY(MemoryInterface, Server::Configuration::BootstrapExtensionFactory);

} // namespace Idxd
} // namespace Buffer
} // namespace Extensions
} // namespace Envoy
