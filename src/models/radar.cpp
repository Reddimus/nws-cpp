// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/radar.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string_view>
#include <utility>
#include <variant>

#include "glaze_detail.hpp"

namespace nws {

namespace {

void populate_radar_station_properties(const glz::generic& props, RadarStationProperties& p) {
	p.id = detail::get_string(props, "id");
	p.name = detail::get_string(props, "name");
	p.station_type = detail::get_string(props, "stationType");

	// Inline coordinates take precedence over GeoJSON geometry, matching
	// the pre-migration order of operations.
	std::optional<double> lat = detail::get_optional_double(props, "latitude");
	std::optional<double> lon = detail::get_optional_double(props, "longitude");
	if (lat) {
		p.latitude = *lat;
	}
	if (lon) {
		p.longitude = *lon;
	}

	const glz::generic* elev = detail::find_object(props, "elevation");
	if (elev != nullptr) {
		detail::parse_quantitative_value(*elev, p.elevation);
	}
}

void populate_radar_station_feature(const glz::generic& root, RadarStationFeature& r) {
	r.id = detail::get_string(root, "id");
	std::string type = detail::get_string(root, "type");
	r.type = type.empty() ? "Feature" : std::move(type);

	const glz::generic* geom = detail::find_object(root, "geometry");
	const glz::generic* props = detail::find_object(root, "properties");

	if (geom != nullptr) {
		r.geometry = detail::parse_geometry(*geom);
	}

	if (props != nullptr) {
		populate_radar_station_properties(*props, r.properties);
	}

	// If the properties block didn't supply lat/lon, fall back to the
	// geometry's Point coordinates. This mirrors the pre-migration logic
	// that landed lat/lon onto the properties struct as a convenience.
	if (props != nullptr && std::holds_alternative<GeoPoint>(r.geometry)) {
		const bool no_inline_lat = detail::get_optional_double(*props, "latitude") == std::nullopt;
		if (no_inline_lat) {
			const GeoPoint& gp = std::get<GeoPoint>(r.geometry);
			r.properties.latitude = gp.latitude;
			r.properties.longitude = gp.longitude;
		}
	}
}

} // namespace

Result<void> deserialize_radar_station_feature(std::string_view body, RadarStationFeature& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	populate_radar_station_feature(*root, out);
	return {};
}

Result<void> deserialize_radar_station_collection(std::string_view body,
												  RadarStationCollectionResponse& out) {
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
		RadarStationFeature station;
		populate_radar_station_feature(feat, station);
		out.features.push_back(std::move(station));
	}
	return {};
}

Result<void> deserialize_radar_server_properties(std::string_view body,
												 RadarServerProperties& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	out.id = detail::get_string(*root, "id");
	out.host = detail::get_string(*root, "host");
	return {};
}

} // namespace nws
