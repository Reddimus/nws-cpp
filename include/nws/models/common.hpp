#pragma once

/// Shared quantitative-value parser. Successful parses replace the destination.

#include "nws/error.hpp"
#include "nws/units.hpp"

#include <string_view>

namespace nws {

[[nodiscard]] Result<void> deserialize_quantitative_value(std::string_view body,
														  QuantitativeValue& out);

} // namespace nws
