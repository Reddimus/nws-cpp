// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/product.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string_view>

#include "glaze_detail.hpp"

namespace nws {

Result<void> deserialize_product_properties(std::string_view body, ProductProperties& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	out.id = detail::get_string(*root, "id");
	out.wmo_collective_id = detail::get_string(*root, "wmoCollectiveId");
	out.issuing_office = detail::get_string(*root, "issuingOffice");
	out.issuance_time = detail::get_string(*root, "issuanceTime");
	out.product_code = detail::get_string(*root, "productCode");
	out.product_name = detail::get_string(*root, "productName");
	out.product_text = detail::get_string(*root, "productText");
	return {};
}

Result<void> deserialize_product_type_properties(std::string_view body,
												 ProductTypeProperties& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	out.type_id = detail::get_string(*root, "productCode");
	out.type_name = detail::get_string(*root, "productName");
	return {};
}

} // namespace nws
