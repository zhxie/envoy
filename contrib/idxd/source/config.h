#pragma once

#include "source/common/buffer/memory_interface.h"

#ifndef IDXD_DISABLED
#include "accel-config/idxd.h"
#include "accel-config/libaccel_config.h"
#endif

namespace Envoy {
namespace Extensions {
namespace Buffer {
namespace Idxd {

class MemoryInterface : public Envoy::Buffer::MemoryInterfaceBase {
public:
  MemoryInterface();
  MemoryInterface(const std::string& device, const std::string& name, uint32_t cutoff,
                  uint64_t wait_cycle);
  ~MemoryInterface() override;

  // Buffer::MemoryInterface
  void memoryCopy(void* dest, const void* src, size_t n) override;
  void batchMemoryCopy(const std::vector<void*>& dests, const std::vector<const void*>& srcs,
                       const std::vector<size_t>& ns) override;

  // Server::Configuration::BootstrapExtensionFactory
  Server::BootstrapExtensionPtr
  createBootstrapExtension(const Protobuf::Message& config,
                           Server::Configuration::ServerFactoryContext& context) override;
  ProtobufTypes::MessagePtr createEmptyConfigProto() override;
  std::string name() const override { return "envoy.extensions.idxd.idxd_memory_interface"; };

private:
  uint32_t cutoff_;
  uint64_t wait_cycle_;

#ifndef IDXD_DISABLED
  accfg_ctx* context_;
  accfg_device* device_;
  accfg_wq* work_queue_;
  void* portal_;

  bool block_on_fault_;
  uint32_t completion_size_;
  accfg_wq_mode mode_;

  completion_record* createCompletionRecord() const;
  hw_desc* createDescriptor(uint64_t record, uint64_t dest, uint64_t src, size_t n) const;
  accfg_wq* getWorkQueueByDevice(const std::string& device) const;
  accfg_wq* getWorkQueueByName(const std::string& name) const;
  uint64_t getWorkQueueSize() const;
  void initialize(const std::string& device, const std::string& name);
  uint8_t submit(const hw_desc* descriptor);
  void wait(completion_record* record) const;
#endif
};

DECLARE_FACTORY(Config);

} // namespace Idxd
} // namespace Buffer
} // namespace Extensions
} // namespace Envoy
