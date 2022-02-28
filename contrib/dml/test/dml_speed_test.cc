#include <dml/hl/status_code.hpp>

#include "source/common/common/assert.h"

#include "absl/strings/string_view.h"
#include "benchmark/benchmark.h"
#include "dml/dml.hpp"

namespace Envoy {

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_memcpy(benchmark::State& state) {
  const std::string data(state.range(0), 'a');
  std::string buffer;
  buffer.reserve(data.size());
  for (auto _ : state) {
    UNREFERENCED_PARAMETER(_);
    memcpy(buffer.data(), data.data(), data.size());
  }
  benchmark::DoNotOptimize(buffer.size());
};
BENCHMARK(BM_memcpy)->Arg(1)->Arg(4096)->Arg(16384)->Arg(65536);

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_DmlSoftware(benchmark::State& state) {
  const std::string data(state.range(0), 'a');
  std::string buffer;
  buffer.resize(data.size());
  for (auto _ : state) {
    UNREFERENCED_PARAMETER(_);
    auto result =
        dml::execute<dml::software>(dml::mem_copy, dml::make_view(data.data(), data.size()),
                                    dml::make_view(buffer.data(), buffer.size()));
    RELEASE_ASSERT(result.status == dml::status_code::ok,
                   fmt::format("DML software mem copy failed with {}", result.status));
    ASSERT(data == buffer, "");
  }
  benchmark::DoNotOptimize(buffer.size());
};
BENCHMARK(BM_DmlSoftware)->Arg(1)->Arg(4096)->Arg(16384)->Arg(65536);

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_DmlHardware(benchmark::State& state) {
  const std::string data(state.range(0), 'a');
  std::string buffer;
  buffer.resize(data.size());
  for (auto _ : state) {
    UNREFERENCED_PARAMETER(_);
    auto result =
        dml::execute<dml::hardware>(dml::mem_copy, dml::make_view(data.data(), data.size()),
                                    dml::make_view(buffer.data(), buffer.size()));
    RELEASE_ASSERT(result.status == dml::status_code::ok,
                   fmt::format("DML hardware mem copy failed with {}", result.status));
    ASSERT(data == buffer, "");
  }
  benchmark::DoNotOptimize(buffer.size());
};
BENCHMARK(BM_DmlHardware)->Arg(1)->Arg(4096)->Arg(16384)->Arg(65536);

} // namespace Envoy
