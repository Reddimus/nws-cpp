// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/forecast.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <string_view>

#include "glaze_detail.hpp"

namespace nws {

namespace {

void populate_forecast_period(const glz::generic& node, ForecastPeriod& p) {
	p.number = detail::get_int(node, "number");
	p.name = detail::get_string(node, "name");
	p.start_time = detail::get_string(node, "startTime");
	p.end_time = detail::get_string(node, "endTime");
	p.is_daytime = detail::get_bool(node, "isDaytime", true);
	p.temperature = detail::get_int(node, "temperature");
	p.temperature_unit = detail::get_string(node, "temperatureUnit");
	p.temperature_trend = detail::get_optional_string(node, "temperatureTrend");

	const glz::generic* pop = detail::find_object(node, "probabilityOfPrecipitation");
	if (pop != nullptr) {
		detail::parse_quantitative_value(*pop, p.probability_of_precipitation);
	}

	// windSpeed: sometimes a string like "10 to 15 mph", sometimes a
	// QuantitativeValue object. The pre-migration code coerced both into a
	// std::string, so we mirror that behaviour exactly.
	if (node.is_object()) {
		glz::generic::object_t::const_iterator ws_it = node.get_object().find("windSpeed");
		if (ws_it != node.get_object().end()) {
			const glz::generic& ws = ws_it->second;
			if (ws.is_string()) {
				p.wind_speed = ws.get<std::string>();
			} else if (ws.is_object()) {
				std::optional<double> v = detail::get_optional_double(ws, "value");
				std::string unit = detail::get_string(ws, "unitCode");
				if (v) {
					p.wind_speed = std::to_string(static_cast<int>(*v));
					if (!unit.empty()) {
						p.wind_speed += " ";
						p.wind_speed += unit;
					}
				}
			}
		}
	}

	p.wind_direction = detail::get_string(node, "windDirection");
	p.icon = detail::get_string(node, "icon");
	p.short_forecast = detail::get_string(node, "shortForecast");
	p.detailed_forecast = detail::get_string(node, "detailedForecast");
}

void populate_forecast_properties(const glz::generic& props, ForecastProperties& p) {
	p.update_time = detail::get_string(props, "updateTime");
	p.generated_at = detail::get_string(props, "generatedAt");

	const glz::generic* elev = detail::find_object(props, "elevation");
	if (elev != nullptr) {
		detail::parse_quantitative_value(*elev, p.elevation);
	}

	const glz::generic* periods = detail::find_array(props, "periods");
	if (periods != nullptr) {
		const glz::generic::array_t& arr = periods->get_array();
		p.periods.clear();
		p.periods.reserve(arr.size());
		for (const glz::generic& period_json : arr) {
			ForecastPeriod period;
			populate_forecast_period(period_json, period);
			p.periods.push_back(std::move(period));
		}
	}
}

} // namespace

Result<void> deserialize_forecast_response(std::string_view body, ForecastResponse& out) {
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
		populate_forecast_properties(*props, out.properties);
	}

	return {};
}

} // namespace nws
