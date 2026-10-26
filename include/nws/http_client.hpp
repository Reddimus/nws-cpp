#pragma once

#include "nws/error.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace nws {

/// HTTP response
/// Headers retain their received order.
struct HttpResponse {
	std::int16_t status_code; // HTTP status codes fit in int16 (100-599)
	std::string body;
	std::vector<std::pair<std::string, std::string>> headers;
};

/// HTTP client configuration
struct ClientConfig {
	std::string base_url{"https://api.weather.gov"};
	std::string user_agent; // REQUIRED: "(appname, contact@email.com)"
	std::string accept{"application/geo+json"};
	std::chrono::seconds timeout{30};
	bool verify_ssl{true};
	/// Maximum decompressed response body; 0 disables the limit.
	std::size_t max_response_bytes{64 * 1024 * 1024};
};

/// HTTP client for NWS API
///
/// GET-only client since the NWS API is read-only.
///
/// @note Thread Safety: This class is NOT thread-safe. The underlying CURL
/// handle is shared across all requests. If you need concurrent API access,
/// create one HttpClient instance per thread or protect access with a mutex.
class HttpClient {
public:
	/// Create a client with the given configuration
	explicit HttpClient(ClientConfig config);
	~HttpClient();

	HttpClient(HttpClient&&) noexcept;
	HttpClient& operator=(HttpClient&&) noexcept;

	// Non-copyable
	HttpClient(const HttpClient&) = delete;
	HttpClient& operator=(const HttpClient&) = delete;

	/// Perform a GET request
	[[nodiscard]] Result<HttpResponse> get(std::string_view path) const;

	/// Get the client configuration
	[[nodiscard]] const ClientConfig& config() const noexcept;

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace nws
