#include "nws/rate_limit.hpp"

#include <algorithm>
#include <chrono>
#include <thread>

namespace nws {
namespace {
RateLimiter::Config sanitized(RateLimiter::Config config) {
	if (config.refill_interval <= std::chrono::milliseconds::zero()) {
		config.refill_interval = std::chrono::milliseconds{1000};
	}
	config.max_tokens = std::max<std::uint16_t>(config.max_tokens, 1);
	config.initial_tokens = std::min(config.initial_tokens, config.max_tokens);
	return config;
}
} // namespace

RateLimiter::RateLimiter(Config config)
	: config_(sanitized(config)), tokens_(config_.initial_tokens), last_refill_(Clock::now()) {}

void RateLimiter::refill() noexcept {
	const Clock::time_point now = Clock::now();
	if (tokens_ >= config_.max_tokens) {
		last_refill_ = now;
		return;
	}
	const std::int64_t earned =
		std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill_).count() /
		config_.refill_interval.count();
	if (earned <= 0) {
		return;
	}
	const std::int64_t total = static_cast<std::int64_t>(tokens_) + earned;
	if (total >= config_.max_tokens) {
		tokens_ = config_.max_tokens;
		last_refill_ = now;
	} else {
		tokens_ = static_cast<std::uint16_t>(total);
		last_refill_ += earned * config_.refill_interval;
	}
}

std::optional<RateLimiter::Clock::duration> RateLimiter::take_or_wait_time() noexcept {
	refill();
	if (tokens_ > 0) {
		--tokens_;
		return std::nullopt;
	}
	const Clock::duration interval =
		config_.refill_interval >=
				std::chrono::duration_cast<std::chrono::milliseconds>(Clock::duration::max())
			? Clock::duration::max()
			: std::chrono::duration_cast<Clock::duration>(config_.refill_interval);
	return std::max(interval - (Clock::now() - last_refill_), Clock::duration::zero());
}

bool RateLimiter::try_acquire() noexcept {
	const std::lock_guard lock(mutex_);
	return !take_or_wait_time().has_value();
}

bool RateLimiter::acquire() {
	return config_.max_wait ? acquire_for(*config_.max_wait) : acquire_until(std::nullopt);
}

bool RateLimiter::acquire_for(std::chrono::milliseconds max_wait) {
	const Clock::time_point now = Clock::now();
	if (max_wait <= std::chrono::milliseconds::zero()) {
		return acquire_until(now);
	}
	const std::chrono::milliseconds remaining =
		std::chrono::duration_cast<std::chrono::milliseconds>(Clock::time_point::max() - now);
	return acquire_until(max_wait >= remaining ? Clock::time_point::max() : now + max_wait);
}

bool RateLimiter::acquire_until(std::optional<Clock::time_point> deadline) {
	while (true) {
		std::optional<Clock::duration> wait;
		{
			const std::lock_guard lock(mutex_);
			wait = take_or_wait_time();
		}
		if (!wait) {
			return true;
		}
		if (deadline && *wait > *deadline - Clock::now()) {
			return false;
		}
		std::this_thread::sleep_for(*wait);
	}
}

std::uint16_t RateLimiter::available_tokens() const noexcept {
	const std::lock_guard lock(mutex_);
	const std::int64_t earned =
		std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - last_refill_).count() /
		config_.refill_interval.count();
	return static_cast<std::uint16_t>(std::min<std::int64_t>(static_cast<std::int64_t>(tokens_) +
																 std::max<std::int64_t>(earned, 0),
															 config_.max_tokens));
}

void RateLimiter::reset() noexcept {
	const std::lock_guard lock(mutex_);
	tokens_ = config_.initial_tokens;
	last_refill_ = Clock::now();
}

const RateLimiter::Config& RateLimiter::config() const noexcept {
	return config_;
}
ScopedRateLimit::ScopedRateLimit(RateLimiter& limiter) : acquired_(limiter.acquire()) {}
bool ScopedRateLimit::acquired() const noexcept {
	return acquired_;
}
ScopedRateLimit::operator bool() const noexcept {
	return acquired_;
}
} // namespace nws
