// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/observation.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string_view>
#include <utility>

#include "glaze_detail.hpp"

namespace nws {

namespace {

void parse_qv_field(const glz::generic& props, const char* key, QuantitativeValue& qv) {
	const glz::generic* node = detail::find_object(props, key);
	if (node == nullptr) {
		return;
	}
	detail::parse_quantitative_value(*node, qv);
}

void populate_observation_properties(const glz::generic& props, ObservationProperties& p) {
	p.id = detail::get_string(props, "@id");
	p.station_url = detail::get_string(props, "station");
	p.station_id = detail::get_string(props, "stationId");
	p.station_name = detail::get_string(props, "stationName");
	p.timestamp = detail::get_string(props, "timestamp");
	p.raw_message = detail::get_string(props, "rawMessage");
	p.text_description = detail::get_string(props, "textDescription");
	p.icon = detail::get_string(props, "icon");

	parse_qv_field(props, "elevation", p.elevation);
	parse_qv_field(props, "temperature", p.temperature);
	parse_qv_field(props, "dewpoint", p.dewpoint);
	parse_qv_field(props, "windDirection", p.wind_direction);
	parse_qv_field(props, "windSpeed", p.wind_speed);
	parse_qv_field(props, "windGust", p.wind_gust);
	parse_qv_field(props, "barometricPressure", p.barometric_pressure);
	parse_qv_field(props, "seaLevelPressure", p.sea_level_pressure);
	parse_qv_field(props, "visibility", p.visibility);
	parse_qv_field(props, "relativeHumidity", p.relative_humidity);
	parse_qv_field(props, "windChill", p.wind_chill);
	parse_qv_field(props, "heatIndex", p.heat_index);
	parse_qv_field(props, "precipitationLastHour", p.precipitation_last_hour);
	parse_qv_field(props, "precipitationLast3Hours", p.precipitation_last_3_hours);
	parse_qv_field(props, "precipitationLast6Hours", p.precipitation_last_6_hours);
	parse_qv_field(props, "maxTemperatureLast24Hours", p.max_temperature_last_24_hours);
	parse_qv_field(props, "minTemperatureLast24Hours", p.min_temperature_last_24_hours);

	const glz::generic* clouds = detail::find_array(props, "cloudLayers");
	if (clouds == nullptr) {
		return;
	}
	const glz::generic::array_t& arr = clouds->get_array();
	p.cloud_layers.reserve(arr.size());
	for (const glz::generic& cl_json : arr) {
		CloudLayer cl;
		const glz::generic* base = detail::find_object(cl_json, "base");
		if (base != nullptr) {
			detail::parse_quantitative_value(*base, cl.base);
		}
		cl.amount = detail::get_string(cl_json, "amount");
		p.cloud_layers.push_back(std::move(cl));
	}
}

void populate_observation_response(const glz::generic& root, ObservationResponse& r) {
	r.id = detail::get_string(root, "id");
	std::string type = detail::get_string(root, "type");
	r.type = type.empty() ? "Feature" : std::move(type);

	const glz::generic* geom = detail::find_object(root, "geometry");
	if (geom != nullptr) {
		r.geometry = detail::parse_geometry(*geom);
	}

	const glz::generic* props = detail::find_object(root, "properties");
	if (props != nullptr) {
		populate_observation_properties(*props, r.properties);
	}
}

} // namespace

Result<void> deserialize_observation_response(std::string_view body, ObservationResponse& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	populate_observation_response(*root, out);
	return {};
}

Result<void> deserialize_observation_collection(std::string_view body,
												ObservationCollectionResponse& out) {
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
		ObservationResponse obs;
		populate_observation_response(feat, obs);
		out.features.push_back(std::move(obs));
	}

	return {};
}

} // namespace nws
