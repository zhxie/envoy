#pragma once

#include "benchmark/benchmark.h"

namespace Envoy {
class Bandwidth : public ::benchmark::Fixture {
public:
  void initialize(::benchmark::State& state) {
    const auto size = static_cast<std::uint32_t>(state.range(0));
    const auto op = static_cast<std::size_t>(state.range(1));
    srcs_ = std::vector<std::vector<std::uint8_t>>(op, std::vector<std::uint8_t>(size, 0u));
    dests_ = std::vector<std::vector<std::uint8_t>>(op, std::vector<std::uint8_t>(size, 0u));

    state.counters["Bandwidth"] = benchmark::Counter(static_cast<double>(size * op),
                                                     benchmark::Counter::kIsIterationInvariantRate,
                                                     benchmark::Counter::kIs1000);
  }

  std::vector<std::vector<std::uint8_t>> srcs_;
  std::vector<std::vector<std::uint8_t>> dests_;
};
} // namespace Envoy
