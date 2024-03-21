#include "contrib/cryptomb/shared_key_providers/source/cryptomb_stats.h"

#include "envoy/stats/scope.h"
#include "envoy/stats/stats_macros.h"

namespace Envoy {
namespace Extensions {
namespace SharedKeyMethodProvider {
namespace CryptoMb {

CryptoMbStats generateCryptoMbStats(const std::string& prefix, Stats::Scope& scope) {
  return CryptoMbStats{ALL_CRYPTOMB_STATS(POOL_HISTOGRAM_PREFIX(scope, prefix))};
}

} // namespace CryptoMb
} // namespace SharedKeyMethodProvider
} // namespace Extensions
} // namespace Envoy
