#include "nws/models/office.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, OfficeProperties& p) {
	p.id = json_string(j, "id");
	p.name = json_string(j, "name");
	p.address = json_string(j, "address");
	p.telephone = json_string(j, "telephone");
	p.fax_number = json_string(j, "faxNumber");
	p.email = json_string(j, "email");
	p.nws_region = json_string(j, "nwsRegion");
	p.parent_org = json_string(j, "parentOrganization");

	if (j.contains("responsibleCounties") && j["responsibleCounties"].is_array()) {
		p.responsible_counties = j["responsibleCounties"].get<std::vector<std::string>>();
	}
}

void from_json(const nlohmann::json& j, HeadlineProperties& p) {
	p.id = json_string(j, "id");
	p.office_id = json_string(j, "office");
	p.title = json_string(j, "title");
	p.summary = json_string(j, "summary");
	p.content = json_string(j, "content");
	p.link = json_string(j, "link");
	p.issuance_time = json_string(j, "issuanceTime");
}

} // namespace nws
