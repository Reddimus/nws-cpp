// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/zone.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string_view>
#include <utility>

#include "glaze_detail.hpp"

namespace nws {

namespace {

void populate_zone_properties(const glz::generic& props, ZoneProperties& p) {
	p.id = detail::get_string(props, "@id");
	p.type = detail::get_string(props, "@type");
	p.name = detail::get_string(props, "name");
	p.state = detail::get_string(props, "state");
	p.forecast_offices = detail::get_string_array(props, "forecastOffices");
	p.observation_stations = detail::get_string_array(props, "observationStations");
}

void populate_zone_feature(const glz::generic& root, ZoneFeature& r) {
	r.id = detail::get_string(root, "id");
	std::string type = detail::get_string(root, "type");
	r.type = type.empty() ? "Feature" : std::move(type);

	const glz::generic* geom = detail::find_object(root, "geometry");
	if (geom != nullptr) {
		r.geometry = detail::parse_geometry(*geom);
	}

	const glz::generic* props = detail::find_object(root, "properties");
	if (props != nullptr) {
		populate_zone_properties(*props, r.properties);
	}
}

void populate_zone_forecast_period(const glz::generic& node, ZoneForecastPeriod& p) {
	p.number = detail::get_int(node, "number");
	p.name = detail::get_string(node, "name");
	p.detailed_forecast = detail::get_string(node, "detailedForecast");
}

} // namespace

Result<void> deserialize_zone_feature(std::string_view body, ZoneFeature& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	populate_zone_feature(*root, out);
	return {};
}

Result<void> deserialize_zone_collection(std::string_view body, ZoneCollectionResponse& out) {
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
		ZoneFeature zone;
		populate_zone_feature(feat, zone);
		out.features.push_back(std::move(zone));
	}

	return {};
}

Result<void> deserialize_zone_forecast(std::string_view body, ZoneForecastProperties& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}

	// The API returns {"properties": {"zone": [...], "periods": [...]}}; the
	// pre-migration api/client.cpp peeled the "properties" key before calling
	// from_json. Preserve that behaviour here for parity (tolerate both
	// wrapped and unwrapped shapes).
	const glz::generic* props = detail::find_object(*root, "properties");
	const glz::generic& target = props != nullptr ? *props : *root;

	out.zone = detail::get_string_array(target, "zone");

	const glz::generic* periods = detail::find_array(target, "periods");
	if (periods != nullptr) {
		const glz::generic::array_t& arr = periods->get_array();
		out.periods.clear();
		out.periods.reserve(arr.size());
		for (const glz::generic& period_json : arr) {
			ZoneForecastPeriod period;
			populate_zone_forecast_period(period_json, period);
			out.periods.push_back(std::move(period));
		}
	}

	return {};
}

} // namespace nws
