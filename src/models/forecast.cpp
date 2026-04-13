#include "nws/models/forecast.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, ForecastPeriod& p) {
	p.number = json_int(j, "number");
	p.name = json_string(j, "name");
	p.start_time = json_string(j, "startTime");
	p.end_time = json_string(j, "endTime");
	p.is_daytime =
		j.contains("isDaytime") && j["isDaytime"].is_boolean() ? j["isDaytime"].get<bool>() : true;
	p.temperature = json_int(j, "temperature");
	p.temperature_unit = json_string(j, "temperatureUnit");

	if (j.contains("temperatureTrend") && j["temperatureTrend"].is_string()) {
		p.temperature_trend = j["temperatureTrend"].get<std::string>();
	}

	if (j.contains("probabilityOfPrecipitation") && !j["probabilityOfPrecipitation"].is_null()) {
		from_json(j["probabilityOfPrecipitation"], p.probability_of_precipitation);
	}

	if (j.contains("windSpeed")) {
		if (j["windSpeed"].is_string()) {
			p.wind_speed = j["windSpeed"].get<std::string>();
		} else if (j["windSpeed"].is_object()) {
			auto qv_val = j["windSpeed"].value("value", 0.0);
			auto qv_unit = j["windSpeed"].value("unitCode", "");
			p.wind_speed = std::to_string(static_cast<int>(qv_val)) + " " + qv_unit;
		}
	}

	p.wind_direction = json_string(j, "windDirection");
	p.icon = json_string(j, "icon");
	p.short_forecast = json_string(j, "shortForecast");
	p.detailed_forecast = json_string(j, "detailedForecast");
}

void from_json(const nlohmann::json& j, ForecastProperties& p) {
	p.update_time = json_string(j, "updateTime");
	p.generated_at = json_string(j, "generatedAt");

	if (j.contains("elevation") && !j["elevation"].is_null()) {
		from_json(j["elevation"], p.elevation);
	}

	if (j.contains("periods") && j["periods"].is_array()) {
		p.periods.clear();
		for (const auto& period_json : j["periods"]) {
			ForecastPeriod period;
			from_json(period_json, period);
			p.periods.push_back(std::move(period));
		}
	}
}

void from_json(const nlohmann::json& j, ForecastResponse& r) {
	r.id = json_string(j, "id");
	r.type = j.contains("type") && j["type"].is_string() ? j["type"].get<std::string>() : "Feature";

	if (j.contains("geometry") && !j["geometry"].is_null()) {
		GeoPoint gp;
		from_json(j["geometry"], gp);
		r.geometry = gp;
	}

	if (j.contains("properties") && !j["properties"].is_null()) {
		from_json(j["properties"], r.properties);
	}
}

} // namespace nws
