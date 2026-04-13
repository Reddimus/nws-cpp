#include "nws/retry.hpp"

#include <gtest/gtest.h>

namespace nws {
namespace {

TEST(RetryPolicyTest, Defaults) {
	RetryPolicy policy;
	EXPECT_EQ(policy.initial_delay, std::chrono::milliseconds(500));
	EXPECT_EQ(policy.max_delay, std::chrono::milliseconds(30000));
	EXPECT_EQ(policy.max_attempts, 3);
	EXPECT_DOUBLE_EQ(policy.backoff_multiplier, 2.0);
	EXPECT_DOUBLE_EQ(policy.jitter_factor, 0.1);
	EXPECT_TRUE(policy.retry_on_network_error);
	EXPECT_TRUE(policy.retry_on_rate_limit);
	EXPECT_TRUE(policy.retry_on_server_error);
}

TEST(ShouldRetryResponseTest, RetryOn503) {
	RetryPolicy policy;
	HttpResponse response{503, "", {}};
	EXPECT_TRUE(should_retry(response, policy));
}

TEST(ShouldRetryResponseTest, RetryOn500) {
	RetryPolicy policy;
	HttpResponse response{500, "", {}};
	EXPECT_TRUE(should_retry(response, policy));
}

TEST(ShouldRetryResponseTest, NoRetryOn200) {
	RetryPolicy policy;
	HttpResponse response{200, "", {}};
	EXPECT_FALSE(should_retry(response, policy));
}

TEST(ShouldRetryResponseTest, NoRetryOn404) {
	RetryPolicy policy;
	HttpResponse response{404, "", {}};
	EXPECT_FALSE(should_retry(response, policy));
}

TEST(ShouldRetryResponseTest, NoRetryOn400) {
	RetryPolicy policy;
	HttpResponse response{400, "", {}};
	EXPECT_FALSE(should_retry(response, policy));
}

TEST(ShouldRetryResponseTest, NoRetryWhenDisabled) {
	RetryPolicy policy;
	policy.retry_on_rate_limit = false;
	HttpResponse response{503, "", {}};
	EXPECT_FALSE(should_retry(response, policy));
}

TEST(ShouldRetryResponseTest, NoRetryServerErrorWhenDisabled) {
	RetryPolicy policy;
	policy.retry_on_server_error = false;
	HttpResponse response{500, "", {}};
	EXPECT_FALSE(should_retry(response, policy));
}

TEST(ShouldRetryErrorTest, RetryOnNetworkError) {
	RetryPolicy policy;
	auto err = Error::network("timeout");
	EXPECT_TRUE(should_retry(err, policy));
}

TEST(ShouldRetryErrorTest, NoRetryOnParseError) {
	RetryPolicy policy;
	auto err = Error::parse("bad json");
	EXPECT_FALSE(should_retry(err, policy));
}

TEST(ShouldRetryErrorTest, NoRetryWhenDisabled) {
	RetryPolicy policy;
	policy.retry_on_network_error = false;
	auto err = Error::network("timeout");
	EXPECT_FALSE(should_retry(err, policy));
}

TEST(CalculateRetryDelayTest, FirstAttempt) {
	RetryPolicy policy;
	policy.jitter_factor = 0; // No jitter for deterministic test
	auto delay = calculate_retry_delay(1, policy);
	EXPECT_EQ(delay, std::chrono::milliseconds(500));
}

TEST(CalculateRetryDelayTest, ExponentialBackoff) {
	RetryPolicy policy;
	policy.jitter_factor = 0;
	auto delay1 = calculate_retry_delay(1, policy);
	auto delay2 = calculate_retry_delay(2, policy);
	auto delay3 = calculate_retry_delay(3, policy);

	EXPECT_EQ(delay1, std::chrono::milliseconds(500));
	EXPECT_EQ(delay2, std::chrono::milliseconds(1000));
	EXPECT_EQ(delay3, std::chrono::milliseconds(2000));
}

TEST(CalculateRetryDelayTest, CapsAtMaxDelay) {
	RetryPolicy policy;
	policy.jitter_factor = 0;
	policy.max_delay = std::chrono::milliseconds(1500);
	auto delay = calculate_retry_delay(3, policy); // Would be 2000ms without cap
	EXPECT_EQ(delay, std::chrono::milliseconds(1500));
}

TEST(CalculateRetryDelayTest, JitterRange) {
	RetryPolicy policy;
	policy.jitter_factor = 0.1;
	// Run multiple times and check delay is within expected range
	for (int i = 0; i < 20; ++i) {
		auto delay = calculate_retry_delay(1, policy);
		EXPECT_GE(delay.count(), 450); // 500 * 0.9
		EXPECT_LE(delay.count(), 550); // 500 * 1.1
	}
}

TEST(WithRetryTest, SuccessOnFirstAttempt) {
	int call_count = 0;
	RetryPolicy policy;
	auto result = with_retry(
		[&]() -> Result<HttpResponse> {
			++call_count;
			return HttpResponse{200, "ok", {}};
		},
		policy);

	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->status_code, 200);
	EXPECT_EQ(call_count, 1);
}

TEST(WithRetryTest, RetriesOnServerError) {
	int call_count = 0;
	RetryPolicy policy;
	policy.initial_delay = std::chrono::milliseconds(1);
	policy.max_attempts = 3;
	policy.jitter_factor = 0;

	auto result = with_retry(
		[&]() -> Result<HttpResponse> {
			++call_count;
			if (call_count < 3) {
				return HttpResponse{500, "error", {}};
			}
			return HttpResponse{200, "ok", {}};
		},
		policy);

	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->status_code, 200);
	EXPECT_EQ(call_count, 3);
}

TEST(WithRetryTest, GivesUpAfterMaxAttempts) {
	int call_count = 0;
	RetryPolicy policy;
	policy.initial_delay = std::chrono::milliseconds(1);
	policy.max_attempts = 2;
	policy.jitter_factor = 0;

	auto result = with_retry(
		[&]() -> Result<HttpResponse> {
			++call_count;
			return HttpResponse{503, "busy", {}};
		},
		policy);

	// After max_attempts, returns the last response (503)
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->status_code, 503);
	EXPECT_EQ(call_count, 2);
}

TEST(WithRetryTest, RetriesOnNetworkError) {
	int call_count = 0;
	RetryPolicy policy;
	policy.initial_delay = std::chrono::milliseconds(1);
	policy.max_attempts = 3;
	policy.jitter_factor = 0;

	auto result = with_retry(
		[&]() -> Result<HttpResponse> {
			++call_count;
			if (call_count < 3) {
				return std::unexpected(Error::network("timeout"));
			}
			return HttpResponse{200, "ok", {}};
		},
		policy);

	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->status_code, 200);
	EXPECT_EQ(call_count, 3);
}

TEST(WithRetryTest, NoRetryOnNonRetryableError) {
	int call_count = 0;
	RetryPolicy policy;
	policy.initial_delay = std::chrono::milliseconds(1);

	auto result = with_retry(
		[&]() -> Result<HttpResponse> {
			++call_count;
			return std::unexpected(Error::parse("bad json"));
		},
		policy);

	ASSERT_FALSE(result.has_value());
	EXPECT_EQ(call_count, 1);
}

} // namespace
} // namespace nws
