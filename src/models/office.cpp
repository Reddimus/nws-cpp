#include "nws/models/office.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, OfficeProperties& p) {
	p.id = j.value("id", "");
	p.name = j.value("name", "");
	p.address = j.value("address", "");
	p.telephone = j.value("telephone", "");
	p.fax_number = j.value("faxNumber", "");
	p.email = j.value("email", "");
	p.nws_region = j.value("nwsRegion", "");
	p.parent_org = j.value("parentOrganization", "");

	if (j.contains("responsibleCounties") && j["responsibleCounties"].is_array()) {
		p.responsible_counties = j["responsibleCounties"].get<std::vector<std::string>>();
	}
}

void from_json(const nlohmann::json& j, HeadlineProperties& p) {
	p.id = j.value("id", "");
	p.office_id = j.value("office", "");
	p.title = j.value("title", "");
	p.summary = j.value("summary", "");
	p.content = j.value("content", "");
	p.link = j.value("link", "");
	p.issuance_time = j.value("issuanceTime", "");
}

} // namespace nws
