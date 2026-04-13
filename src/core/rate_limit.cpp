#include "nws/rate_limit.hpp"

namespace nws {

RateLimiter::RateLimiter(Config config)
	: config_(std::move(config)), tokens_(config_.initial_tokens),
	  last_refill_(std::chrono::steady_clock::now()) {}

void RateLimiter::refill() noexcept {
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill_);

	if (elapsed >= config_.refill_interval) {
		auto new_tokens =
			static_cast<std::uint16_t>(elapsed.count() / config_.refill_interval.count());
		tokens_ = std::min(static_cast<std::uint16_t>(tokens_ + new_tokens), config_.max_tokens);
		last_refill_ = now;
	}
}

bool RateLimiter::try_acquire() noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	refill();
	if (tokens_ > 0) {
		--tokens_;
		return true;
	}
	return false;
}

bool RateLimiter::acquire() {
	if (config_.max_wait) {
		return acquire_for(*config_.max_wait);
	}

	// No max_wait: block indefinitely
	while (true) {
		if (try_acquire()) {
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

bool RateLimiter::acquire_for(std::chrono::milliseconds max_wait) {
	auto deadline = std::chrono::steady_clock::now() + max_wait;

	while (std::chrono::steady_clock::now() < deadline) {
		if (try_acquire()) {
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return false;
}

std::uint16_t RateLimiter::available_tokens() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return tokens_;
}

void RateLimiter::reset() noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	tokens_ = config_.initial_tokens;
	last_refill_ = std::chrono::steady_clock::now();
}

const RateLimiter::Config& RateLimiter::config() const noexcept {
	return config_;
}

// ScopedRateLimit

ScopedRateLimit::ScopedRateLimit(RateLimiter& limiter) : acquired_(limiter.acquire()) {}

bool ScopedRateLimit::acquired() const noexcept {
	return acquired_;
}

ScopedRateLimit::operator bool() const noexcept {
	return acquired_;
}

} // namespace nws
