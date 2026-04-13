#pragma once

#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <vector>

namespace nws {

/// Properties for a Weather Forecast Office
struct OfficeProperties {
	std::string id;
	std::string name;
	std::string address;
	std::string telephone;
	std::string fax_number;
	std::string email;
	std::string nws_region;
	std::string parent_org;
	std::vector<std::string> responsible_counties;
};

/// Properties for an office headline
struct HeadlineProperties {
	std::string id;
	std::string office_id;
	std::string title;
	std::string summary;
	std::string content;
	std::string link;
	std::string issuance_time;
};

void from_json(const nlohmann::json& j, OfficeProperties& p);
void from_json(const nlohmann::json& j, HeadlineProperties& p);

} // namespace nws
