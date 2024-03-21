#pragma once

#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"

namespace Envoy {
namespace Extensions {
namespace SharedKeyMethodProvider {
namespace CryptoMb {

#define ALL_CRYPTOMB_STATS(HISTOGRAM) HISTOGRAM(x25519_queue_sizes, Unspecified)

/**
 * CryptoMb stats struct definition. @see stats_macros.h
 */
struct CryptoMbStats {
  ALL_CRYPTOMB_STATS(GENERATE_HISTOGRAM_STRUCT)
};

CryptoMbStats generateCryptoMbStats(const std::string& prefix, Stats::Scope& scope);

} // namespace CryptoMb
} // namespace SharedKeyMethodProvider
} // namespace Extensions
} // namespace Envoy
