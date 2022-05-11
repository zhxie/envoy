#include "test/mocks/server/factory_context.h"

#include "contrib/envoy/extensions/idxd/v3alpha/idxd_memory_interface.pb.h"
#include "contrib/idxd/source/config.h"

namespace Envoy {
namespace Extensions {
namespace Buffer {
namespace Idxd {

// To run following tests, make sure at least one DSA work queue named "envoy" is enabled.
constexpr absl::string_view yaml = R"EOF(
name: envoy
cutoff: {}
)EOF";

constexpr size_t size = 1024;
constexpr size_t op = 8;

class MemoryInterfaceTest : public ::testing::Test {
protected:
  void setup(uint32_t cutoff) {
    srcs_ = std::vector<std::vector<uint8_t>>(op, std::vector<uint8_t>(size, 1u));
    dests_ = std::vector<std::vector<uint8_t>>(op, std::vector<uint8_t>(size, 0u));

    envoy::extensions::idxd::v3alpha::IdxdMemoryInterface config;
    TestUtility::loadFromYaml(fmt::format(std::string(yaml), cutoff), config);
    memory_interface_.createBootstrapExtension(config, context_);
  }

  NiceMock<Server::Configuration::MockServerFactoryContext> context_;
  MemoryInterface memory_interface_;
  std::vector<std::vector<uint8_t>> srcs_;
  std::vector<std::vector<uint8_t>> dests_;

  void memoryCopy() {
    memory_interface_.memoryCopy(dests_[0].data(), srcs_[0].data(), size);
    EXPECT_EQ(dests_[0], srcs_[0]);
  }

  void batchMemoryCopy() {
    std::vector<void*> dests;
    std::vector<const void*> srcs;
    std::vector<size_t> ns;
    for (size_t i = 0; i < op; i++) {
      dests.push_back(dests_[i].data());
      srcs.push_back(srcs_[i].data());
      ns.push_back(size);
    }
    memory_interface_.batchMemoryCopy(dests, srcs, ns);
    EXPECT_EQ(srcs_, dests_);
  }
};

#ifdef DML_DISABLED
// Verify that incompatible architecture will cause a throw.
TEST_F(MemoryInterfaceTest, IncompatibleArchitecture) {
  EXPECT_THROW_WITH_MESSAGE(setup(0, 0), EnvoyException,
                            "X86_64 architecture is required for IDXD.");
#else
// Verify that memory copy can be offloaded to DSA.
TEST_F(MemoryInterfaceTest, HardwarePath) {
  setup(0);
  EXPECT_LOG_NOT_CONTAINS("warning", "failed", memoryCopy());
}

// Verify that un-met memory copy will fallback to default memory copy.
TEST_F(MemoryInterfaceTest, Fallback) {
  setup(size + 1);
  EXPECT_LOG_NOT_CONTAINS("trace", "copy memory", memoryCopy());
}

// Verify that batch memory copy can be offloaded to DSA.
TEST_F(MemoryInterfaceTest, BatchHardwarePath) {
  setup(0);
  EXPECT_LOG_NOT_CONTAINS("warning", "failed", batchMemoryCopy());
}

// Verify that un-met batch memory copy will fallback to default memory copy.
TEST_F(MemoryInterfaceTest, BatchFallback) {
  setup(size + 1);
  EXPECT_LOG_NOT_CONTAINS("trace", "copy memory", batchMemoryCopy());
}
#endif

} // namespace Idxd
} // namespace Buffer
} // namespace Extensions
} // namespace Envoy
