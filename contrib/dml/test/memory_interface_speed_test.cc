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

BENCHMARK_DEFINE_F(Bandwidth, DefaultBatch)(::benchmark::State& state) {
  initialize(state);
  Buffer::MemoryInterfaceImpl memory_interface;
  std::vector<const void*> srcs;
  std::vector<void*> dests;
  std::vector<size_t> ns;
  for (const auto& src : srcs_) {
    srcs.push_back(src.data());
    ns.push_back(src.size());
  }
  for (auto& dest : dests_) {
    dests.push_back(dest.data());
  }
  for (auto _ : state) { // NOLINT
    memory_interface.batchMemoryCopy(dests, srcs, ns);
  }
}

BENCHMARK_DEFINE_F(Bandwidth, DmlBatch)(::benchmark::State& state) {
  initialize(state);
  Extensions::Buffer::Dml::MemoryInterface memory_interface;
  std::vector<const void*> srcs;
  std::vector<void*> dests;
  std::vector<size_t> ns;
  for (const auto& src : srcs_) {
    srcs.push_back(src.data());
    ns.push_back(src.size());
  }
  for (auto& dest : dests_) {
    dests.push_back(dest.data());
  }
  for (auto _ : state) { // NOLINT
    memory_interface.batchMemoryCopy(dests, srcs, ns);
  }
}

BENCHMARK_REGISTER_F(Bandwidth, Default)->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {1}});
BENCHMARK_REGISTER_F(Bandwidth, Dml)->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {1}});
BENCHMARK_REGISTER_F(Bandwidth, DefaultBatch)
    ->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {4, 16, 128}});
BENCHMARK_REGISTER_F(Bandwidth, DmlBatch)
    ->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {4, 16, 128}});

} // namespace Envoy
