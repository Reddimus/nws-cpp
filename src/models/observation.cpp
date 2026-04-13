#include "nws/models/observation.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

namespace {

void parse_qv(const nlohmann::json& j, const std::string& key, QuantitativeValue& qv) {
	if (j.contains(key) && !j[key].is_null()) {
		from_json(j[key], qv);
	}
}

} // namespace

void from_json(const nlohmann::json& j, CloudLayer& cl) {
	if (j.contains("base") && !j["base"].is_null()) {
		from_json(j["base"], cl.base);
	}
	cl.amount = json_string(j, "amount");
}

void from_json(const nlohmann::json& j, ObservationProperties& p) {
	p.id = json_string(j, "@id");
	p.station_url = json_string(j, "station");
	p.station_id = json_string(j, "stationId");
	p.station_name = json_string(j, "stationName");
	p.timestamp = json_string(j, "timestamp");
	p.raw_message = json_string(j, "rawMessage");
	p.text_description = json_string(j, "textDescription");
	p.icon = json_string(j, "icon");

	parse_qv(j, "elevation", p.elevation);
	parse_qv(j, "temperature", p.temperature);
	parse_qv(j, "dewpoint", p.dewpoint);
	parse_qv(j, "windDirection", p.wind_direction);
	parse_qv(j, "windSpeed", p.wind_speed);
	parse_qv(j, "windGust", p.wind_gust);
	parse_qv(j, "barometricPressure", p.barometric_pressure);
	parse_qv(j, "seaLevelPressure", p.sea_level_pressure);
	parse_qv(j, "visibility", p.visibility);
	parse_qv(j, "relativeHumidity", p.relative_humidity);
	parse_qv(j, "windChill", p.wind_chill);
	parse_qv(j, "heatIndex", p.heat_index);
	parse_qv(j, "precipitationLastHour", p.precipitation_last_hour);
	parse_qv(j, "precipitationLast3Hours", p.precipitation_last_3_hours);
	parse_qv(j, "precipitationLast6Hours", p.precipitation_last_6_hours);
	parse_qv(j, "maxTemperatureLast24Hours", p.max_temperature_last_24_hours);
	parse_qv(j, "minTemperatureLast24Hours", p.min_temperature_last_24_hours);

	if (j.contains("cloudLayers") && j["cloudLayers"].is_array()) {
		for (const auto& cl_json : j["cloudLayers"]) {
			CloudLayer cl;
			from_json(cl_json, cl);
			p.cloud_layers.push_back(std::move(cl));
		}
	}
}

void from_json(const nlohmann::json& j, ObservationResponse& r) {
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

void from_json(const nlohmann::json& j, ObservationCollectionResponse& r) {
	r.type = j.contains("type") && j["type"].is_string() ? j["type"].get<std::string>()
														 : "FeatureCollection";
	if (j.contains("features") && j["features"].is_array()) {
		for (const auto& feat : j["features"]) {
			ObservationResponse obs;
			from_json(feat, obs);
			r.features.push_back(std::move(obs));
		}
	}
}

} // namespace nws
