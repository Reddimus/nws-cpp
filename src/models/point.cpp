#include "nws/models/point.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, PointProperties& p) {
	p.id = json_string(j, "@id");
	p.grid_id = json_string(j, "gridId");
	p.grid_x = json_int(j, "gridX");
	p.grid_y = json_int(j, "gridY");
	p.forecast_url = json_string(j, "forecast");
	p.forecast_hourly_url = json_string(j, "forecastHourly");
	p.forecast_grid_data_url = json_string(j, "forecastGridData");
	p.observation_stations_url = json_string(j, "observationStations");
	p.forecast_zone_url = json_string(j, "forecastZone");
	p.county_url = json_string(j, "county");
	p.fire_weather_zone_url = json_string(j, "fireWeatherZone");
	p.time_zone = json_string(j, "timeZone");
	p.radar_station = json_string(j, "radarStation");

	if (j.contains("relativeLocation") && !j["relativeLocation"].is_null()) {
		const nlohmann::json& rl = j["relativeLocation"];
		if (rl.contains("geometry") && !rl["geometry"].is_null()) {
			GeoPoint gp;
			from_json(rl["geometry"], gp);
			p.relative_location.geometry = gp;
		}
		if (rl.contains("properties") && !rl["properties"].is_null()) {
			const nlohmann::json& props = rl["properties"];
			p.relative_location.properties.city = json_string(props, "city");
			p.relative_location.properties.state = json_string(props, "state");
			if (props.contains("distance") && !props["distance"].is_null()) {
				from_json(props["distance"], p.relative_location.properties.distance);
			}
			if (props.contains("bearing") && !props["bearing"].is_null()) {
				from_json(props["bearing"], p.relative_location.properties.bearing);
			}
		}
	}
}

void from_json(const nlohmann::json& j, PointResponse& r) {
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
