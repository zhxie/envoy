// Note: this should be run with --compilation_mode=opt, and would benefit from
// a quiescent system with disabled cstate power management.

#include "source/common/common/assert.h"

#include "benchmark/benchmark.h"
#include "dml/dml.hpp"

namespace Envoy {

class MemoryInterfaceBenchmarkFixture : public ::benchmark::Fixture {
public:
  void initialize(::benchmark::State& state) {
    const std::size_t size = static_cast<std::uint32_t>(state.range(0));
    src = std::vector<std::uint8_t>(size, 0u);
    dest = std::vector<std::uint8_t>(size, 0u);

    state.counters["Bandwidth"] =
        benchmark::Counter(static_cast<double>(size), benchmark::Counter::kIsIterationInvariantRate,
                           benchmark::Counter::kIs1000);
  }

  std::vector<std::uint8_t> src;
  std::vector<std::uint8_t> dest;
};

BENCHMARK_DEFINE_F(MemoryInterfaceBenchmarkFixture, Default)(::benchmark::State& state) {
  initialize(state);
  for (auto _ : state) { // NOLINT
    memcpy(dest.data(), src.data(), src.size());
  }
}

BENCHMARK_DEFINE_F(MemoryInterfaceBenchmarkFixture, DmlSoftware)(::benchmark::State& state) {
  initialize(state);
  for (auto _ : state) { // NOLINT
    const auto result =
        dml::execute<dml::software>(dml::mem_copy, dml::make_view(src.data(), src.size()),
                                    dml::make_view(dest.data(), dest.size()));
    const auto code = result.status;
    if (code != dml::status_code::ok) {
      state.SkipWithError(
          fmt::format("Operation failed with errno {}!", static_cast<uint32_t>(code)).c_str());
    }
  }
}

BENCHMARK_DEFINE_F(MemoryInterfaceBenchmarkFixture, DmlHardware)(::benchmark::State& state) {
  initialize(state);
  for (auto _ : state) { // NOLINT
    const auto result =
        dml::execute<dml::hardware>(dml::mem_copy, dml::make_view(src.data(), src.size()),
                                    dml::make_view(dest.data(), dest.size()));
    const auto code = result.status;
    if (code != dml::status_code::ok) {
      state.SkipWithError(
          fmt::format("Operation failed with errno {}!", static_cast<uint32_t>(code)).c_str());
    }
  }
}

BENCHMARK_REGISTER_F(MemoryInterfaceBenchmarkFixture, Default)
    ->RangeMultiplier(4)
    ->Ranges({
        {1, 1048576},
    });
BENCHMARK_REGISTER_F(MemoryInterfaceBenchmarkFixture, DmlSoftware)
    ->RangeMultiplier(4)
    ->Ranges({
        {1, 1048576},
    });
BENCHMARK_REGISTER_F(MemoryInterfaceBenchmarkFixture, DmlHardware)
    ->RangeMultiplier(4)
    ->Ranges({
        {1, 1048576},
    });

} // namespace Envoy
