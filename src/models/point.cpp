// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/point.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string_view>

#include "glaze_detail.hpp"

namespace nws {

namespace {

void populate_point_properties(const glz::generic& props, PointProperties& p) {
	p.id = detail::get_string(props, "@id");
	p.grid_id = detail::get_string(props, "gridId");
	p.grid_x = detail::get_int(props, "gridX");
	p.grid_y = detail::get_int(props, "gridY");
	p.forecast_url = detail::get_string(props, "forecast");
	p.forecast_hourly_url = detail::get_string(props, "forecastHourly");
	p.forecast_grid_data_url = detail::get_string(props, "forecastGridData");
	p.observation_stations_url = detail::get_string(props, "observationStations");
	p.forecast_zone_url = detail::get_string(props, "forecastZone");
	p.county_url = detail::get_string(props, "county");
	p.fire_weather_zone_url = detail::get_string(props, "fireWeatherZone");
	p.time_zone = detail::get_string(props, "timeZone");
	p.radar_station = detail::get_string(props, "radarStation");

	const glz::generic* rl = detail::find_object(props, "relativeLocation");
	if (rl != nullptr) {
		const glz::generic* geom = detail::find_object(*rl, "geometry");
		if (geom != nullptr) {
			p.relative_location.geometry = detail::parse_geometry(*geom);
		}
		const glz::generic* rl_props = detail::find_object(*rl, "properties");
		if (rl_props != nullptr) {
			p.relative_location.properties.city = detail::get_string(*rl_props, "city");
			p.relative_location.properties.state = detail::get_string(*rl_props, "state");
			const glz::generic* dist = detail::find_object(*rl_props, "distance");
			if (dist != nullptr) {
				detail::parse_quantitative_value(*dist, p.relative_location.properties.distance);
			}
			const glz::generic* bearing = detail::find_object(*rl_props, "bearing");
			if (bearing != nullptr) {
				detail::parse_quantitative_value(*bearing, p.relative_location.properties.bearing);
			}
		}
	}
}

} // namespace

Result<void> deserialize_point_response(std::string_view body, PointResponse& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}

	out.id = detail::get_string(*root, "id");
	std::string type = detail::get_string(*root, "type");
	out.type = type.empty() ? "Feature" : std::move(type);

	const glz::generic* geom = detail::find_object(*root, "geometry");
	if (geom != nullptr) {
		out.geometry = detail::parse_geometry(*geom);
	}

	const glz::generic* props = detail::find_object(*root, "properties");
	if (props != nullptr) {
		populate_point_properties(*props, out.properties);
	}

	return {};
}

} // namespace nws
