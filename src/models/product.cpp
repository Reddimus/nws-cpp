#include "nws/models/product.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, ProductProperties& p) {
	p.id = json_string(j, "id");
	p.wmo_collective_id = json_string(j, "wmoCollectiveId");
	p.issuing_office = json_string(j, "issuingOffice");
	p.issuance_time = json_string(j, "issuanceTime");
	p.product_code = json_string(j, "productCode");
	p.product_name = json_string(j, "productName");
	p.product_text = json_string(j, "productText");
}

void from_json(const nlohmann::json& j, ProductTypeProperties& p) {
	p.type_id = json_string(j, "productCode");
	p.type_name = json_string(j, "productName");
}

} // namespace nws
