#include "nws/error.hpp"

#include <nlohmann/json.hpp>

namespace nws {

Error Error::from_response(int status, const std::string& body, const std::string& correlation_id) {
	Error err;
	err.http_status = status;
	err.correlation_id = correlation_id;

	// Determine error code from HTTP status
	if (status == 404) {
		err.code = ErrorCode::NotFound;
	} else if (status == 400) {
		err.code = ErrorCode::InvalidRequest;
	} else if (status == 503) {
		err.code = ErrorCode::RateLimited;
	} else if (status >= 500) {
		err.code = ErrorCode::ServerError;
	} else {
		err.code = ErrorCode::Unknown;
	}

	// Try to parse RFC 7807 Problem Details JSON
	try {
		auto j = nlohmann::json::parse(body);
		err.message = j.value("title", "");
		err.detail = j.value("detail", "");
		if (err.correlation_id.empty()) {
			err.correlation_id = j.value("correlationId", "");
		}
	} catch (...) {
		// Not valid JSON — use raw body as message
		err.message = body.substr(0, 256);
	}

	if (err.message.empty()) {
		err.message = "HTTP " + std::to_string(status);
	}

	return err;
}

} // namespace nws
