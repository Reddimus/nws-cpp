#pragma once

#include "nws/error.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nws {

/// Properties for a text product
struct ProductProperties {
	std::string id;
	std::string wmo_collective_id;
	std::string issuing_office;
	std::string issuance_time;
	std::string product_code;
	std::string product_name;
	std::string product_text;
};

/// Properties for a product type listing
struct ProductTypeProperties {
	std::string type_id;
	std::string type_name;
};

[[nodiscard]] Result<void> deserialize_product_properties(std::string_view body,
														  ProductProperties& out);
[[nodiscard]] Result<void> deserialize_product_type_properties(std::string_view body,
															   ProductTypeProperties& out);

} // namespace nws
