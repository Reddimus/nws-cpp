// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/common.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string_view>

#include "glaze_detail.hpp"

namespace nws {

Result<void> deserialize_quantitative_value(std::string_view body, QuantitativeValue& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	detail::parse_quantitative_value(*root, out);
	return {};
}

} // namespace nws
