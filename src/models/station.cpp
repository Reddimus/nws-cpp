#include "nws/models/station.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, StationProperties& p) {
	p.id = json_string(j, "@id");
	p.station_identifier = json_string(j, "stationIdentifier");
	p.name = json_string(j, "name");
	p.time_zone = json_string(j, "timeZone");
	p.forecast_url = json_string(j, "forecast");
	p.county_url = json_string(j, "county");
	p.fire_weather_zone_url = json_string(j, "fireWeatherZone");

	if (j.contains("elevation") && !j["elevation"].is_null()) {
		from_json(j["elevation"], p.elevation);
	}
}

void from_json(const nlohmann::json& j, StationResponse& r) {
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

void from_json(const nlohmann::json& j, StationCollectionResponse& r) {
	r.type = j.contains("type") && j["type"].is_string() ? j["type"].get<std::string>()
														 : "FeatureCollection";
	if (j.contains("features") && j["features"].is_array()) {
		for (const auto& feat : j["features"]) {
			StationResponse station;
			from_json(feat, station);
			r.features.push_back(std::move(station));
		}
	}
}

} // namespace nws
