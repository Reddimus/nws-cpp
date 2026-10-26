#pragma once

#include <string>
#include <string_view>

namespace nws::detail {

// RFC 3986 query-component encoding; the cursor is opaque, not a query fragment.
[[nodiscard]] inline std::string encode_query_value(std::string_view value) {
	constexpr char hex[] = "0123456789ABCDEF";
	std::string encoded;
	encoded.reserve(value.size());
	for (const unsigned char byte : value) {
		if ((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
			(byte >= '0' && byte <= '9') || byte == '-' || byte == '.' || byte == '_' ||
			byte == '~') {
			encoded += static_cast<char>(byte);
		} else {
			encoded += '%';
			encoded += hex[byte >> 4];
			encoded += hex[byte & 15];
		}
	}
	return encoded;
}

} // namespace nws::detail
