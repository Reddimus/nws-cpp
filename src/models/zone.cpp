#include "nws/models/zone.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, ZoneProperties& p) {
	p.id = j.value("@id", "");
	p.type = j.value("@type", "");
	p.name = j.value("name", "");
	p.state = j.value("state", "");

	if (j.contains("forecastOffices") && j["forecastOffices"].is_array()) {
		p.forecast_offices = j["forecastOffices"].get<std::vector<std::string>>();
	}
	if (j.contains("observationStations") && j["observationStations"].is_array()) {
		p.observation_stations = j["observationStations"].get<std::vector<std::string>>();
	}
}

void from_json(const nlohmann::json& j, ZoneFeature& r) {
	r.id = j.value("id", "");
	r.type = j.value("type", "Feature");

	if (j.contains("geometry") && !j["geometry"].is_null()) {
		GeoPoint gp;
		from_json(j["geometry"], gp);
		r.geometry = gp;
	}

	if (j.contains("properties") && !j["properties"].is_null()) {
		from_json(j["properties"], r.properties);
	}
}

void from_json(const nlohmann::json& j, ZoneCollectionResponse& r) {
	r.type = j.value("type", "FeatureCollection");
	if (j.contains("features") && j["features"].is_array()) {
		for (const auto& feat : j["features"]) {
			ZoneFeature zone;
			from_json(feat, zone);
			r.features.push_back(std::move(zone));
		}
	}
}

void from_json(const nlohmann::json& j, ZoneForecastPeriod& p) {
	p.number = j.value("number", 0);
	p.name = j.value("name", "");
	p.detailed_forecast = j.value("detailedForecast", "");
}

void from_json(const nlohmann::json& j, ZoneForecastProperties& p) {
	if (j.contains("zone") && j["zone"].is_array()) {
		p.zone = j["zone"].get<std::vector<std::string>>();
	}

	if (j.contains("periods") && j["periods"].is_array()) {
		p.periods.clear();
		for (const auto& period_json : j["periods"]) {
			ZoneForecastPeriod period;
			from_json(period_json, period);
			p.periods.push_back(std::move(period));
		}
	}
}

} // namespace nws
