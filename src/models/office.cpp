// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/office.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string_view>

#include "glaze_detail.hpp"

namespace nws {

Result<void> deserialize_office_properties(std::string_view body, OfficeProperties& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	out.id = detail::get_string(*root, "id");
	out.name = detail::get_string(*root, "name");
	out.address = detail::get_string(*root, "address");
	out.telephone = detail::get_string(*root, "telephone");
	out.fax_number = detail::get_string(*root, "faxNumber");
	out.email = detail::get_string(*root, "email");
	out.nws_region = detail::get_string(*root, "nwsRegion");
	out.parent_org = detail::get_string(*root, "parentOrganization");
	out.responsible_counties = detail::get_string_array(*root, "responsibleCounties");
	return {};
}

Result<void> deserialize_headline_properties(std::string_view body, HeadlineProperties& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	out.id = detail::get_string(*root, "id");
	out.office_id = detail::get_string(*root, "office");
	out.title = detail::get_string(*root, "title");
	out.summary = detail::get_string(*root, "summary");
	out.content = detail::get_string(*root, "content");
	out.link = detail::get_string(*root, "link");
	out.issuance_time = detail::get_string(*root, "issuanceTime");
	return {};
}

} // namespace nws
