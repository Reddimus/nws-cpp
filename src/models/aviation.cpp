// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/aviation.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string_view>
#include <utility>

#include "glaze_detail.hpp"

namespace nws {

namespace {

void populate_sigmet_properties(const glz::generic& props, SigmetProperties& p) {
	p.id = detail::get_string(props, "id");
	p.atsu = detail::get_string(props, "atsu");
	p.sequence = detail::get_string(props, "sequence");
	p.phenomenon = detail::get_string(props, "phenomenon");
	p.valid_time_from = detail::get_string(props, "validTimeFrom");
	p.valid_time_to = detail::get_string(props, "validTimeTo");
}

void populate_sigmet_feature(const glz::generic& root, SigmetFeature& r) {
	r.id = detail::get_string(root, "id");
	std::string type = detail::get_string(root, "type");
	r.type = type.empty() ? "Feature" : std::move(type);

	const glz::generic* geom = detail::find_object(root, "geometry");
	if (geom != nullptr) {
		r.geometry = detail::parse_geometry(*geom);
	}

	const glz::generic* props = detail::find_object(root, "properties");
	if (props != nullptr) {
		populate_sigmet_properties(*props, r.properties);
	}
}

void populate_cwa_properties(const glz::generic& props, CWAProperties& p) {
	p.id = detail::get_string(props, "id");
	p.cwsu = detail::get_string(props, "cwsu");
	p.sequence = detail::get_string(props, "sequence");
	p.text = detail::get_string(props, "text");
	p.valid_time_from = detail::get_string(props, "validTimeFrom");
	p.valid_time_to = detail::get_string(props, "validTimeTo");
}

void populate_cwa_feature(const glz::generic& root, CWAFeature& r) {
	r.id = detail::get_string(root, "id");
	std::string type = detail::get_string(root, "type");
	r.type = type.empty() ? "Feature" : std::move(type);

	const glz::generic* geom = detail::find_object(root, "geometry");
	if (geom != nullptr) {
		r.geometry = detail::parse_geometry(*geom);
	}

	const glz::generic* props = detail::find_object(root, "properties");
	if (props != nullptr) {
		populate_cwa_properties(*props, r.properties);
	}
}

} // namespace

Result<void> deserialize_sigmet_feature(std::string_view body, SigmetFeature& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	populate_sigmet_feature(*root, out);
	return {};
}

Result<void> deserialize_sigmet_collection(std::string_view body, SigmetCollectionResponse& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}

	std::string type = detail::get_string(*root, "type");
	out.type = type.empty() ? "FeatureCollection" : std::move(type);

	const glz::generic* features = detail::find_array(*root, "features");
	if (features == nullptr) {
		return {};
	}
	const glz::generic::array_t& arr = features->get_array();
	out.features.reserve(arr.size());
	for (const glz::generic& feat : arr) {
		SigmetFeature sigmet;
		populate_sigmet_feature(feat, sigmet);
		out.features.push_back(std::move(sigmet));
	}
	return {};
}

Result<void> deserialize_cwa_feature(std::string_view body, CWAFeature& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	populate_cwa_feature(*root, out);
	return {};
}

Result<void> deserialize_cwa_collection(std::string_view body, CWACollectionResponse& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}

	std::string type = detail::get_string(*root, "type");
	out.type = type.empty() ? "FeatureCollection" : std::move(type);

	const glz::generic* features = detail::find_array(*root, "features");
	if (features == nullptr) {
		return {};
	}
	const glz::generic::array_t& arr = features->get_array();
	out.features.reserve(arr.size());
	for (const glz::generic& feat : arr) {
		CWAFeature cwa;
		populate_cwa_feature(feat, cwa);
		out.features.push_back(std::move(cwa));
	}
	return {};
}

} // namespace nws
