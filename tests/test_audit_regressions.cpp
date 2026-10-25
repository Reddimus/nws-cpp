#include "nws/models/cli_report.hpp"
#include "nws/nws.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <gtest/gtest.h>
#include <limits>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "support/loopback_server.hpp"

namespace {
using namespace std::chrono_literals;
using nws::test::http_response;
using nws::test::LoopbackRequest;
using nws::test::LoopbackServer;

class LocalEnvironment : public testing::Environment {
	void SetUp() override {
#ifdef _WIN32
		(void)::_putenv_s("NO_PROXY", "127.0.0.1");
		(void)::_putenv_s("no_proxy", "127.0.0.1");
#else
		(void)::setenv("NO_PROXY", "127.0.0.1", 1);
		(void)::setenv("no_proxy", "127.0.0.1", 1);
#endif
	}
};
[[maybe_unused]] testing::Environment* const kEnvironment =
	testing::AddGlobalTestEnvironment(new LocalEnvironment);

nws::NWSClient::Config local(const LoopbackServer& server) {
	nws::NWSClient::Config config;
	config.http.base_url = server.url("");
	config.http.user_agent = "nws-cpp-test";
	return config;
}

TEST(AuditRegression, AlertCountsIgnoreNumbersOutsideInt32) {
	nws::AlertActiveCount counts;
	ASSERT_TRUE(nws::deserialize_alert_active_count(R"({"regions":{"bad":1e100,"ok":3}})", counts));
	EXPECT_EQ(counts.regions.size(), 1u);
	EXPECT_EQ(counts.regions.at("ok"), 3);
}

TEST(AuditRegression, CliUsesCaseInsensitiveLabelsAndDateAfterMonth) {
	const nws::Result<nws::CliReport> result =
		nws::parse_cli_product_text("CLIMATE SUMMARY FOR 24 HOURS ENDING MAY 16 "
									"2026\nTEMPERATURE\n  1) Maximum 72\nMinimum 40\n");
	ASSERT_TRUE(result);
	EXPECT_EQ(result->date, "2026-05-16");
	EXPECT_EQ(result->tmax_f, 72);
	EXPECT_EQ(result->tmin_f, 40);
}

TEST(AuditRegression, ZeroWaitCanTakeAvailableTokenAndExtremeWaitDoesNotOverflow) {
	nws::RateLimiter limiter({.max_tokens = 2, .initial_tokens = 2});
	EXPECT_TRUE(limiter.acquire_for(0ms));
	EXPECT_TRUE(limiter.acquire_for(std::chrono::milliseconds::max()));
}

TEST(AuditRegression, InvalidRateConfigIsNormalized) {
	nws::RateLimiter limiter({.max_tokens = 0, .refill_interval = 0ms, .initial_tokens = 10});
	EXPECT_GT(limiter.config().refill_interval.count(), 0);
	EXPECT_EQ(limiter.available_tokens(), 1);
	EXPECT_TRUE(limiter.try_acquire());
	EXPECT_FALSE(limiter.try_acquire());
}

TEST(AuditRegression, AvailableTokensIncludeRefills) {
	nws::RateLimiter limiter({.max_tokens = 2, .refill_interval = 1ms, .initial_tokens = 0});
	std::this_thread::sleep_for(5ms);
	EXPECT_EQ(limiter.available_tokens(), 2);
}

TEST(AuditRegression, RateLimitTimeoutPreventsHttpRequest) {
	LoopbackServer server{[](const LoopbackRequest&) { return http_response("200 OK", "{}"); }};
	nws::NWSClient::Config config = local(server);
	config.rate_limit.initial_tokens = 0;
	config.rate_limit.max_wait = 0ms;
	nws::NWSClient client(config);
	const nws::Result<nws::PointResponse> result = client.get_point(40, -100);
	ASSERT_FALSE(result);
	EXPECT_EQ(result.error().code, nws::ErrorCode::RateLimited);
	EXPECT_TRUE(server.requests().empty());
}

TEST(AuditRegression, CachedPointRetainsItsFullResponse) {
	LoopbackServer server{[](const LoopbackRequest&) {
		return http_response(
			"200 OK",
			R"({"id":"point-id","geometry":{"type":"Point","coordinates":[-100,40]},"properties":{"gridId":"TOP"}})");
	}};
	nws::NWSClient::Config config = local(server);
	config.enable_cache = true;
	nws::NWSClient client(config);
	const nws::Result<nws::PointResponse> first = client.get_point(40, -100);
	const nws::Result<nws::PointResponse> cached = client.get_point(40, -100);
	ASSERT_TRUE(first);
	ASSERT_TRUE(cached);
	EXPECT_EQ(cached->id, first->id);
	EXPECT_TRUE(std::holds_alternative<nws::GeoPoint>(cached->geometry));
	EXPECT_EQ(server.requests().size(), 1u);
}

TEST(AuditRegression, InvalidPointCoordinatesNeverReachNetwork) {
	LoopbackServer server{[](const LoopbackRequest&) { return http_response("200 OK", "{}"); }};
	nws::NWSClient client(local(server));
	EXPECT_FALSE(client.get_point(std::numeric_limits<double>::quiet_NaN(), 0));
	EXPECT_FALSE(client.get_point(91, 0));
	EXPECT_FALSE(client.get_point(0, 181));
	EXPECT_TRUE(server.requests().empty());
}

TEST(AuditRegression, ZeroCapacityCacheStaysEmptyAndNanKeysRemainEvictable) {
	nws::LruCache<int, int> disabled({.max_entries = 0});
	disabled.put(1, 2);
	EXPECT_EQ(disabled.size(), 0u);
	nws::LruCache<nws::CoordinateKey, int> cache({.max_entries = 1});
	cache.put({std::numeric_limits<double>::quiet_NaN(), 0}, 1);
	cache.put({0, 0}, 2);
	EXPECT_EQ(cache.size(), 1u);
	EXPECT_EQ(cache.get({0, 0}), 2);
}

TEST(AuditRegression, AlertTypesRejectTrailingJson) {
	LoopbackServer server{[](const LoopbackRequest&) {
		return http_response("200 OK", R"({"eventTypes":["Flood"]}garbage)");
	}};
	nws::NWSClient client(local(server));
	const nws::Result<std::vector<std::string>> result = client.get_alert_types();
	ASSERT_FALSE(result);
	EXPECT_EQ(result.error().code, nws::ErrorCode::ParseError);
}

TEST(AuditRegression, UnitConversionDropsUnsupportedValuesAndConvertsBounds) {
	nws::QuantitativeValue unknown;
	unknown.value = 10;
	EXPECT_FALSE(unknown.converted_to(nws::Unit::DegF).value);
	nws::QuantitativeValue bounds;
	bounds.unit = nws::Unit::DegC;
	bounds.min_value = 0;
	bounds.max_value = 100;
	const nws::QuantitativeValue converted = bounds.converted_to(nws::Unit::DegF);
	EXPECT_EQ(converted.unit, nws::Unit::DegF);
	EXPECT_EQ(converted.min_value, 32);
	EXPECT_EQ(converted.max_value, 212);
	ASSERT_TRUE(nws::convert(1, nws::Unit::Mile, nws::Unit::Meter));
	EXPECT_NEAR(*nws::convert(1, nws::Unit::Mile, nws::Unit::Meter), 1609.344, 0.01);
}

TEST(AuditRegression, ReusingModelOutputReplacesPriorData) {
	nws::AlertCollectionResponse collection;
	ASSERT_TRUE(nws::deserialize_alert_collection(R"({"features":[{"id":"old"}]})", collection));
	ASSERT_TRUE(nws::deserialize_alert_collection(R"({"features":[]})", collection));
	EXPECT_TRUE(collection.features.empty());
	nws::QuantitativeValue value;
	ASSERT_TRUE(nws::deserialize_quantitative_value(R"({"value":12})", value));
	ASSERT_TRUE(nws::deserialize_quantitative_value("{}", value));
	EXPECT_FALSE(value.value);
}

TEST(AuditRegression, PathArgumentsAreEncodedAndCorrelationHeaderIgnoresCase) {
	LoopbackServer server{[](const LoopbackRequest&) {
		return http_response("404 Not Found", "{}",
							 "x-CoRrElAtIoN-iD: correlation\r\nX-Empty: \r\n");
	}};
	nws::NWSClient client(local(server));
	const nws::Result<nws::StationResponse> result = client.get_station("K/A?x=1#");
	ASSERT_FALSE(result);
	EXPECT_EQ(result.error().correlation_id, "correlation");
	EXPECT_EQ(server.requests().at(0).target, "/stations/K%2FA%3Fx%3D1%23");
}

TEST(AuditRegression, HttpBodyLimitIsExactAndCanBeDisabled) {
	LoopbackServer server{
		[](const LoopbackRequest&) { return http_response("200 OK", std::string(128, 'x')); }};
	nws::ClientConfig config;
	config.base_url = server.url("");
	config.max_response_bytes = 128;
	EXPECT_TRUE(nws::HttpClient(config).get("/"));
	config.max_response_bytes = 127;
	const nws::Result<nws::HttpResponse> limited = nws::HttpClient(config).get("/");
	ASSERT_FALSE(limited);
	EXPECT_EQ(limited.error().code, nws::ErrorCode::InvalidRequest);
	config.max_response_bytes = 0;
	EXPECT_TRUE(nws::HttpClient(config).get("/"));
}

TEST(AuditRegression, HttpRedirectRetainsOnlyFinalHeadersAndTrimsEmptyValues) {
	LoopbackServer server{[](const LoopbackRequest& request) {
		return request.target == "/first"
				   ? http_response("302 Found", "", "Location: /final\r\nX-Old: ignored\r\n")
				   : http_response("200 OK", "final", "X-Empty: \r\n");
	}};
	nws::ClientConfig config;
	config.base_url = server.url("");
	const nws::Result<nws::HttpResponse> result = nws::HttpClient(config).get("/first");
	ASSERT_TRUE(result);
	EXPECT_EQ(result->body, "final");
	bool saw_empty = false;
	for (const auto& [key, value] : result->headers) {
		EXPECT_NE(key, "X-Old");
		if (key == "X-Empty") {
			saw_empty = true;
			EXPECT_TRUE(value.empty());
		}
	}
	EXPECT_TRUE(saw_empty);
}

TEST(AuditRegression, HttpRejectsInvalidHeadersAndEmbeddedNulBeforeNetwork) {
	LoopbackServer server{[](const LoopbackRequest&) { return http_response("200 OK", "{}"); }};
	nws::ClientConfig config;
	config.base_url = server.url("");
	config.accept = "application/json\r\nX-Injected: yes";
	EXPECT_FALSE(nws::HttpClient(config).get("/"));
	config.accept = "application/json";
	EXPECT_FALSE(nws::HttpClient(config).get(std::string("/x\0hidden", 9)));
	EXPECT_TRUE(server.requests().empty());
}

TEST(AuditRegression, RetryPolicyRejectsNonHttpStatusesAndBoundsInvalidSettings) {
	nws::RetryPolicy policy;
	EXPECT_FALSE(nws::should_retry(nws::HttpResponse{600, "", {}}, policy));
	EXPECT_TRUE(nws::should_retry(nws::HttpResponse{429, "", {}}, policy));
	policy.max_delay = -1ms;
	EXPECT_EQ(nws::calculate_retry_delay(1, policy), 0ms);
	policy.max_delay = std::chrono::milliseconds::max();
	policy.initial_delay = policy.max_delay;
	policy.jitter_factor = std::numeric_limits<double>::infinity();
	EXPECT_EQ(nws::calculate_retry_delay(1, policy), policy.max_delay);
}

TEST(AuditRegression, AlertArrayFiltersUseCommaSeparatedValues) {
	LoopbackServer server{
		[](const LoopbackRequest&) { return http_response("200 OK", R"({"features":[]})"); }};
	nws::NWSClient client(local(server));
	nws::GetAlertsParams params;
	params.area = std::vector<std::string>{"KS", "OK"};
	params.status = std::vector<std::string>{"actual", "test"};
	const nws::Result<nws::AlertCollectionResponse> result = client.get_alerts(params);
	ASSERT_TRUE(result);
	const std::string target = server.requests().at(0).target;
	EXPECT_NE(target.find("area=KS,OK"), std::string::npos);
	EXPECT_NE(target.find("status=actual,test"), std::string::npos);
	EXPECT_EQ(target.find("&area="), target.rfind("&area="));
}

} // namespace
