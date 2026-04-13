#include "nws/models/zone.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, ZoneProperties& p) {
	p.id = json_string(j, "@id");
	p.type = json_string(j, "@type");
	p.name = json_string(j, "name");
	p.state = json_string(j, "state");

	if (j.contains("forecastOffices") && j["forecastOffices"].is_array()) {
		p.forecast_offices = j["forecastOffices"].get<std::vector<std::string>>();
	}
	if (j.contains("observationStations") && j["observationStations"].is_array()) {
		p.observation_stations = j["observationStations"].get<std::vector<std::string>>();
	}
}

void from_json(const nlohmann::json& j, ZoneFeature& r) {
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

void from_json(const nlohmann::json& j, ZoneCollectionResponse& r) {
	r.type = j.contains("type") && j["type"].is_string() ? j["type"].get<std::string>()
														 : "FeatureCollection";
	if (j.contains("features") && j["features"].is_array()) {
		for (const auto& feat : j["features"]) {
			ZoneFeature zone;
			from_json(feat, zone);
			r.features.push_back(std::move(zone));
		}
	}
}

void from_json(const nlohmann::json& j, ZoneForecastPeriod& p) {
	p.number = json_int(j, "number");
	p.name = json_string(j, "name");
	p.detailed_forecast = json_string(j, "detailedForecast");
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
