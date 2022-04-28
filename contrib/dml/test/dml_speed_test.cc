// Note: this should be run with --compilation_mode=opt, and would benefit from
// a quiescent system with disabled cstate power management.

#include "source/common/common/assert.h"

#include "contrib/dml/test/bandwidth.h"
#include "dml/dml.hpp"

namespace Envoy {

BENCHMARK_DEFINE_F(Bandwidth, Default)(::benchmark::State& state) {
  initialize(state);
  const auto op = static_cast<std::size_t>(state.range(1));
  for (auto _ : state) { // NOLINT
    for (std::size_t i = 0; i < op; i++) {
      memcpy(dests_[i].data(), srcs_[i].data(), srcs_[i].size());
    }
  }
}

BENCHMARK_DEFINE_F(Bandwidth, DmlSoftware)(::benchmark::State& state) {
  initialize(state);
  const auto op = static_cast<std::size_t>(state.range(1));
  for (auto _ : state) { // NOLINT
    for (std::size_t i = 0; i < op; i++) {
      const auto result = dml::execute<dml::software>(
          dml::mem_copy, dml::make_view(srcs_[i].data(), srcs_[i].size()),
          dml::make_view(dests_[i].data(), dests_[i].size()));
      const auto code = result.status;
      if (code != dml::status_code::ok) {
        state.SkipWithError(
            fmt::format("Operation failed with errno {}!", static_cast<uint32_t>(code)).c_str());
      }
    }
  }
}

BENCHMARK_DEFINE_F(Bandwidth, DmlSoftwarePipeline)(::benchmark::State& state) {
  initialize(state);
  const auto op = static_cast<std::size_t>(state.range(1));
  for (auto _ : state) { // NOLINT
    std::vector<dml::handler<dml::mem_copy_operation, std::allocator<unsigned char>>> handlers;
    handlers.reserve(op);
    for (std::size_t i = 0; i < op; i++) {
      handlers.push_back(dml::submit<dml::software>(
          dml::mem_copy, dml::make_view(srcs_[i].data(), srcs_[i].size()),
          dml::make_view(dests_[i].data(), dests_[i].size())));
    }
    for (auto& handler : handlers) {
      const auto code = handler.get().status;
      if (code != dml::status_code::ok) {
        state.SkipWithError(
            fmt::format("Operation failed with errno {}!", static_cast<uint32_t>(code)).c_str());
      }
    }
  }
}

BENCHMARK_DEFINE_F(Bandwidth, DmlSoftwareBatch)(::benchmark::State& state) {
  initialize(state);
  const auto op = static_cast<std::size_t>(state.range(1));
  for (auto _ : state) { // NOLINT
    auto sequence = dml::sequence(op, std::allocator<unsigned char>());
    for (std::size_t i = 0; i < op; i++) {
      sequence.add(dml::mem_copy, dml::make_view(srcs_[i].data(), srcs_[i].size()),
                   dml::make_view(dests_[i].data(), dests_[i].size()));
    }
    const auto result = dml::execute<dml::software>(dml::batch, sequence);
    const auto code = result.status;
    if (code != dml::status_code::ok) {
      state.SkipWithError(
          fmt::format("Operation failed with errno {}!", static_cast<uint32_t>(code)).c_str());
    }
  }
}

BENCHMARK_DEFINE_F(Bandwidth, DmlHardware)(::benchmark::State& state) {
  initialize(state);
  const auto op = static_cast<std::size_t>(state.range(1));
  for (auto _ : state) { // NOLINT
    for (std::size_t i = 0; i < op; i++) {
      const auto result = dml::execute<dml::hardware>(
          dml::mem_copy, dml::make_view(srcs_[i].data(), srcs_[i].size()),
          dml::make_view(dests_[i].data(), dests_[i].size()));
      const auto code = result.status;
      if (code != dml::status_code::ok) {
        state.SkipWithError(
            fmt::format("Operation failed with errno {}!", static_cast<uint32_t>(code)).c_str());
      }
    }
  }
}

BENCHMARK_DEFINE_F(Bandwidth, DmlHardwarePipeline)(::benchmark::State& state) {
  initialize(state);
  const auto op = static_cast<std::size_t>(state.range(1));
  for (auto _ : state) { // NOLINT
    std::vector<dml::handler<dml::mem_copy_operation, std::allocator<unsigned char>>> handlers;
    handlers.reserve(op);
    for (std::size_t i = 0; i < op; i++) {
      handlers.push_back(dml::submit<dml::hardware>(
          dml::mem_copy, dml::make_view(srcs_[i].data(), srcs_[i].size()),
          dml::make_view(dests_[i].data(), dests_[i].size())));
    }
    for (auto& handler : handlers) {
      const auto code = handler.get().status;
      if (code != dml::status_code::ok) {
        state.SkipWithError(
            fmt::format("Operation failed with errno {}!", static_cast<uint32_t>(code)).c_str());
      }
    }
  }
}

BENCHMARK_DEFINE_F(Bandwidth, DmlHardwareBatch)(::benchmark::State& state) {
  initialize(state);
  const auto op = static_cast<std::size_t>(state.range(1));
  for (auto _ : state) { // NOLINT
    auto sequence = dml::sequence(op, std::allocator<unsigned char>());
    for (std::size_t i = 0; i < op; i++) {
      sequence.add(dml::mem_copy, dml::make_view(srcs_[i].data(), srcs_[i].size()),
                   dml::make_view(dests_[i].data(), dests_[i].size()));
    }
    const auto result = dml::execute<dml::hardware>(dml::batch, sequence);
    const auto code = result.status;
    if (code != dml::status_code::ok) {
      state.SkipWithError(
          fmt::format("Operation failed with errno {}!", static_cast<uint32_t>(code)).c_str());
    }
  }
}

BENCHMARK_REGISTER_F(Bandwidth, Default)
    ->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {1, 4, 16, 128}});
BENCHMARK_REGISTER_F(Bandwidth, DmlSoftware)
    ->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {1, 4, 16, 128}});
BENCHMARK_REGISTER_F(Bandwidth, DmlSoftwarePipeline)
    ->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {1, 4, 16, 128}});
BENCHMARK_REGISTER_F(Bandwidth, DmlSoftwareBatch)
    ->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {1, 4, 16, 128}});
BENCHMARK_REGISTER_F(Bandwidth, DmlHardware)
    ->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {1, 4, 16, 128}});
BENCHMARK_REGISTER_F(Bandwidth, DmlHardwarePipeline)
    ->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {1, 4, 16, 128}});
BENCHMARK_REGISTER_F(Bandwidth, DmlHardwareBatch)
    ->ArgsProduct({{256, 2048, 16384, 131072, 1048576}, {1, 4, 16, 128}});

} // namespace Envoy
