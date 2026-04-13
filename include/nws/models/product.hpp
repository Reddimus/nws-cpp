#pragma once

#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
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

void from_json(const nlohmann::json& j, ProductProperties& p);
void from_json(const nlohmann::json& j, ProductTypeProperties& p);

} // namespace nws
