// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/gridpoint.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string_view>
#include <utility>

#include "glaze_detail.hpp"

namespace nws {

namespace {

void populate_layer(const glz::generic& node, GridpointLayer& layer) {
	layer.uom = detail::get_string(node, "uom");
	layer.unit = parse_unit_code(layer.uom);

	const glz::generic* values = detail::find_array(node, "values");
	if (values == nullptr) {
		return;
	}
	const glz::generic::array_t& arr = values->get_array();
	layer.values.clear();
	layer.values.reserve(arr.size());
	for (const glz::generic& v : arr) {
		GridpointTimeValue tv;
		tv.valid_time = detail::get_string(v, "validTime");
		tv.value = detail::get_optional_double(v, "value");
		layer.values.push_back(std::move(tv));
	}
}

void maybe_layer(const glz::generic& props, const char* key,
				 std::optional<GridpointLayer>& target) {
	const glz::generic* node = detail::find_object(props, key);
	if (node == nullptr) {
		return;
	}
	GridpointLayer layer;
	populate_layer(*node, layer);
	target = std::move(layer);
}

void populate_gridpoint_properties(const glz::generic& props, GridpointProperties& p) {
	p.update_time = detail::get_string(props, "updateTime");
	p.valid_times = detail::get_string(props, "validTimes");
	p.grid_id = detail::get_string(props, "gridId");
	p.grid_x = detail::get_int(props, "gridX");
	p.grid_y = detail::get_int(props, "gridY");

	const glz::generic* elev = detail::find_object(props, "elevation");
	if (elev != nullptr) {
		detail::parse_quantitative_value(*elev, p.elevation);
	}

	maybe_layer(props, "temperature", p.temperature);
	maybe_layer(props, "dewpoint", p.dewpoint);
	maybe_layer(props, "maxTemperature", p.max_temperature);
	maybe_layer(props, "minTemperature", p.min_temperature);
	maybe_layer(props, "relativeHumidity", p.relative_humidity);
	maybe_layer(props, "apparentTemperature", p.apparent_temperature);
	maybe_layer(props, "heatIndex", p.heat_index);
	maybe_layer(props, "windChill", p.wind_chill);
	maybe_layer(props, "skyCover", p.sky_cover);
	maybe_layer(props, "windDirection", p.wind_direction);
	maybe_layer(props, "windSpeed", p.wind_speed);
	maybe_layer(props, "windGust", p.wind_gust);
	maybe_layer(props, "probabilityOfPrecipitation", p.probability_of_precipitation);
	maybe_layer(props, "quantitativePrecipitation", p.quantitative_precipitation);
	maybe_layer(props, "iceAccumulation", p.ice_accumulation);
	maybe_layer(props, "snowfallAmount", p.snowfall_amount);
	maybe_layer(props, "ceilingHeight", p.ceiling_height);
	maybe_layer(props, "visibility", p.visibility);
	maybe_layer(props, "pressure", p.pressure);
}

} // namespace

Result<void> deserialize_gridpoint_response(std::string_view body, GridpointResponse& out) {
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
		populate_gridpoint_properties(*props, out.properties);
	}

	return {};
}

} // namespace nws
