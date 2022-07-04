// Note: this should be run with --compilation_mode=opt, and would benefit from
// a quiescent system with disabled cstate power management.

#include "source/common/common/assert.h"

#include "benchmark/benchmark.h"
#include "hs/hs.h"
#include "re2/re2.h"

namespace Envoy {

const std::vector<std::string>& clusterInputs() {
  CONSTRUCT_ON_FIRST_USE(
      std::vector<std::string>,
      {
          "cluster.no_trailing_dot",
          "cluster.match.",
          "cluster.match.normal",
          "cluster.match.and.a.whole.lot.of.things.coming.after.the.matches.really.too.much.stuff",
      });
}

constexpr absl::string_view cluster_re_pattern = "^cluster\\.((.*?)\\.)";

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_RE2(benchmark::State& state) {
  re2::RE2 re(std::string(cluster_re_pattern).c_str());
  uint32_t passes = 0;
  for (auto _ : state) { // NOLINT
    for (const std::string& cluster_input : clusterInputs()) {
      if (re2::RE2::FullMatch(cluster_input, re)) {
        ++passes;
      }
    }
  }
  RELEASE_ASSERT(passes > 0, "");
}
BENCHMARK(BM_RE2);

// NOLINTNEXTLINE(readability-identifier-naming)
static void BM_Hyperscan(benchmark::State& state) {
  hs_database_t* database{};
  hs_scratch_t* scratch{};
  hs_compile_error_t* compile_err;
  RELEASE_ASSERT(
      hs_compile_multi(std::vector<const char*>{std::string(cluster_re_pattern).c_str()}.data(),
                       nullptr, nullptr, 1, HS_MODE_BLOCK, nullptr, &database,
                       &compile_err) == HS_SUCCESS,
      "");
  RELEASE_ASSERT(hs_alloc_scratch(database, &scratch) == HS_SUCCESS, "");
  uint32_t passes = 0;
  for (auto _ : state) { // NOLINT
    for (const std::string& cluster_input : clusterInputs()) {
      hs_error_t err = hs_scan(
          database, cluster_input.data(), cluster_input.size(), 0, scratch,
          [](unsigned int, unsigned long long, unsigned long long, unsigned int,
             void* context) -> int {
            uint32_t* passes = static_cast<uint32_t*>(context);
            *passes = *passes + 1;

            return 1;
          },
          &passes);
      ASSERT(err == HS_SUCCESS || err == HS_SCAN_TERMINATED);
    }
  }
  hs_free_scratch(scratch);
  hs_free_database(database);
  RELEASE_ASSERT(passes > 0, "");
}
BENCHMARK(BM_Hyperscan);

} // namespace Envoy
