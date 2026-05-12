// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/station.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string_view>
#include <utility>

#include "glaze_detail.hpp"

namespace nws {

namespace {

void populate_station_properties(const glz::generic& props, StationProperties& p) {
	p.id = detail::get_string(props, "@id");
	p.station_identifier = detail::get_string(props, "stationIdentifier");
	p.name = detail::get_string(props, "name");
	p.time_zone = detail::get_string(props, "timeZone");
	p.forecast_url = detail::get_string(props, "forecast");
	p.county_url = detail::get_string(props, "county");
	p.fire_weather_zone_url = detail::get_string(props, "fireWeatherZone");

	const glz::generic* elev = detail::find_object(props, "elevation");
	if (elev != nullptr) {
		detail::parse_quantitative_value(*elev, p.elevation);
	}
}

void populate_station_response(const glz::generic& root, StationResponse& r) {
	r.id = detail::get_string(root, "id");
	std::string type = detail::get_string(root, "type");
	r.type = type.empty() ? "Feature" : std::move(type);

	const glz::generic* geom = detail::find_object(root, "geometry");
	if (geom != nullptr) {
		r.geometry = detail::parse_geometry(*geom);
	}

	const glz::generic* props = detail::find_object(root, "properties");
	if (props != nullptr) {
		populate_station_properties(*props, r.properties);
	}
}

} // namespace

Result<void> deserialize_station_response(std::string_view body, StationResponse& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	populate_station_response(*root, out);
	return {};
}

Result<void> deserialize_station_collection(std::string_view body, StationCollectionResponse& out) {
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
		StationResponse station;
		populate_station_response(feat, station);
		out.features.push_back(std::move(station));
	}

	return {};
}

} // namespace nws
