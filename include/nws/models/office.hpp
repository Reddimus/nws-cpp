#pragma once

#include "nws/error.hpp"

#include <optional>
#include <string>
#include <string_view>
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

[[nodiscard]] Result<void> deserialize_office_properties(std::string_view body,
														 OfficeProperties& out);
[[nodiscard]] Result<void> deserialize_headline_properties(std::string_view body,
														   HeadlineProperties& out);

} // namespace nws
