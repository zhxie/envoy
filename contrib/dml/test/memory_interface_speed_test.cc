// Note: this should be run with --compilation_mode=opt, and would benefit from
// a quiescent system with disabled cstate power management.

#include "source/common/buffer/memory_interface_impl.h"

#include "contrib/dml/source/config.h"
#include "contrib/dml/test/bandwidth.h"

namespace Envoy {

BENCHMARK_DEFINE_F(Bandwidth, Default)(::benchmark::State& state) {
  initialize(state);
  Buffer::MemoryInterfaceImpl memory_interface;
  for (auto _ : state) { // NOLINT
    memory_interface.memoryCopy(dests_[0].data(), srcs_[0].data(), srcs_[0].size());
  }
}

BENCHMARK_DEFINE_F(Bandwidth, Dml)(::benchmark::State& state) {
  initialize(state);
  Extensions::Buffer::Dml::MemoryInterface memory_interface;
  for (auto _ : state) { // NOLINT
    memory_interface.memoryCopy(dests_[0].data(), srcs_[0].data(), srcs_[0].size());
  }
}

BENCHMARK_REGISTER_F(Bandwidth, Default)->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {1}});
BENCHMARK_REGISTER_F(Bandwidth, Dml)->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {1}});

} // namespace Envoy
