#include "nws/models/product.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, ProductProperties& p) {
	p.id = j.value("id", "");
	p.wmo_collective_id = j.value("wmoCollectiveId", "");
	p.issuing_office = j.value("issuingOffice", "");
	p.issuance_time = j.value("issuanceTime", "");
	p.product_code = j.value("productCode", "");
	p.product_name = j.value("productName", "");
	p.product_text = j.value("productText", "");
}

void from_json(const nlohmann::json& j, ProductTypeProperties& p) {
	p.type_id = j.value("productCode", "");
	p.type_name = j.value("productName", "");
}

} // namespace nws
