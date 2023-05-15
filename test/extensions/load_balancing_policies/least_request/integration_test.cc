#include <chrono>
#include <cstdint>

#include "envoy/config/endpoint/v3/endpoint_components.pb.h"

#include "source/common/common/base64.h"
#include "source/common/http/utility.h"
#include "source/common/protobuf/protobuf.h"
#include "source/extensions/load_balancing_policies/least_request/config.h"

#include "test/integration/http_integration.h"

#include "gtest/gtest.h"

namespace Envoy {
namespace Extensions {
namespace LoadBalancingPolices {
namespace LeastRequest {
namespace {

class LeastRequestIntegrationTest
    : public testing::TestWithParam<
          std::tuple<Network::Address::IpVersion, Network::DefaultSocketInterface>>,
      public HttpIntegrationTest {
public:
  LeastRequestIntegrationTest()
      : HttpIntegrationTest(Http::CodecType::HTTP1, std::get<0>(GetParam()),
                            std::get<1>(GetParam())) {
    // Create 3 different upstream server for stateful session test.
    setUpstreamCount(3);

    // Update endpoints of default cluster `cluster_0` to 3 different fake upstreams.
    config_helper_.addConfigModifier([](envoy::config::bootstrap::v3::Bootstrap& bootstrap) {
      auto* cluster_0 = bootstrap.mutable_static_resources()->mutable_clusters()->Mutable(0);
      ASSERT(cluster_0->name() == "cluster_0");
      auto* endpoint = cluster_0->mutable_load_assignment()->mutable_endpoints()->Mutable(0);

      constexpr absl::string_view endpoints_yaml = R"EOF(
        lb_endpoints:
        - endpoint:
            address:
              socket_address:
                address: {}
                port_value: 0
        - endpoint:
            address:
              socket_address:
                address: {}
                port_value: 0
        - endpoint:
            address:
              socket_address:
                address: {}
                port_value: 0
      )EOF";

      const std::string local_address =
          Network::Test::getLoopbackAddressString(std::get<0>(GetParam()));
      TestUtility::loadFromYaml(
          fmt::format(endpoints_yaml, local_address, local_address, local_address), *endpoint);

      auto* policy = cluster_0->mutable_load_balancing_policy();

      const std::string policy_yaml = R"EOF(
        policies:
        - typed_extension_config:
            name: envoy.load_balancing_policies.least_request
            typed_config:
                "@type": type.googleapis.com/envoy.extensions.load_balancing_policies.least_request.v3.LeastRequest
       )EOF";

      TestUtility::loadFromYaml(policy_yaml, *policy);
    });
  }
};

INSTANTIATE_TEST_SUITE_P(
    IpVersions, LeastRequestIntegrationTest,
    testing::Combine(testing::ValuesIn(TestEnvironment::getIpVersionsForTest()),
                     testing::ValuesIn(TestEnvironment::getSocketInterfacesForTest())),
    TestUtility::ipAndSocketInterfaceTestParamsToString);

TEST_P(LeastRequestIntegrationTest, NormalLoadBalancing) {
  initialize();

  for (uint64_t i = 0; i < 8; i++) {
    codec_client_ = makeHttpConnection(lookupPort("http"));

    Http::TestRequestHeaderMapImpl request_headers{
        {":method", "GET"}, {":path", "/"}, {":scheme", "http"}, {":authority", "example.com"}};

    auto response = codec_client_->makeRequestWithBody(request_headers, 0);

    auto upstream_index = waitForNextUpstreamRequest({0, 1, 2});
    ASSERT(upstream_index.has_value());

    upstream_request_->encodeHeaders(default_response_headers_, true);

    ASSERT_TRUE(response->waitForEndStream());

    EXPECT_TRUE(upstream_request_->complete());
    EXPECT_TRUE(response->complete());

    cleanupUpstreamAndDownstream();
  }
}

} // namespace
} // namespace LeastRequest
} // namespace LoadBalancingPolices
} // namespace Extensions
} // namespace Envoy
