#include "source/common/io/io_uring_factory_impl.h"
#include "source/common/io/io_uring_impl.h"

#include "test/mocks/server/mocks.h"
#include "test/test_common/environment.h"
#include "test/test_common/utility.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Io {
namespace {

class IoUringFactoryImplTest : public ::testing::Test {
public:
  IoUringFactoryImplTest() : api_(Api::createApiForTest()) {
    if (!isIoUringSupported()) {
      should_skip_ = true;
    }
  }

  void SetUp() override {
    if (should_skip_) {
      GTEST_SKIP();
    }
  }

  Api::ApiPtr api_;
  testing::NiceMock<Server::Configuration::MockServerFactoryContext> context_;
  std::unique_ptr<IoUringFactoryImpl> factory_{};
  bool should_skip_{};
};

TEST_F(IoUringFactoryImplTest, Basic) {
  IoUringFactoryImpl factory(2, false, context_.threadLocal());
  EXPECT_TRUE(factory.currentThreadRegistered());
  auto dispatcher = api_->allocateDispatcher("test_thread");
  factory.onServerInitialized();
  EXPECT_TRUE(factory.getIoUringWorker().has_value());
}

} // namespace
} // namespace Io
} // namespace Envoy
