#pragma once

#include <glaze/glaze.hpp>

namespace nws::detail {
struct JsonReadOptions : glz::opts {
	bool validate_trailing_whitespace = true;
};
} // namespace nws::detail
