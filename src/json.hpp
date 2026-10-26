#pragma once

#include <glaze/glaze.hpp>

namespace nws::detail {
struct JsonReadOptions : glz::opts {
	bool validate_trailing_whitespace = true;
};

// Public deserializers accept bounded views, which need no trailing NUL.
inline constexpr JsonReadOptions kJsonReadOptions{{.null_terminated = false}};
} // namespace nws::detail
