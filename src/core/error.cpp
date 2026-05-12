#include "nws/error.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>

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

	// Try to parse RFC 7807 Problem Details JSON. We use glz::generic here
	// (rather than a static glz::meta<ProblemDetails>) because the same
	// helper handles arbitrary error envelopes — title/detail/correlationId
	// may be present, missing, or null, and there's no schema we want to
	// reject on.
	glz::generic root{};
	glz::error_ctx ec = glz::read_json(root, body);
	if (!ec && root.is_object()) {
		const glz::generic::object_t& obj = root.get_object();
		glz::generic::object_t::const_iterator it;
		it = obj.find("title");
		if (it != obj.end() && it->second.is_string()) {
			err.message = it->second.get<std::string>();
		}
		it = obj.find("detail");
		if (it != obj.end() && it->second.is_string()) {
			err.detail = it->second.get<std::string>();
		}
		if (err.correlation_id.empty()) {
			it = obj.find("correlationId");
			if (it != obj.end() && it->second.is_string()) {
				err.correlation_id = it->second.get<std::string>();
			}
		}
	} else {
		// Not valid JSON — use raw body as message
		err.message = body.substr(0, 256);
	}

	if (err.message.empty()) {
		err.message = "HTTP " + std::to_string(status);
	}

	return err;
}

} // namespace nws
