#include "nws/models/gridpoint.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

namespace {

void parse_layer(const nlohmann::json& j, const std::string& key,
				 std::optional<GridpointLayer>& layer) {
	if (j.contains(key) && !j[key].is_null()) {
		GridpointLayer l;
		from_json(j[key], l);
		layer = std::move(l);
	}
}

} // namespace

void from_json(const nlohmann::json& j, GridpointTimeValue& tv) {
	tv.valid_time = json_string(j, "validTime");
	if (j.contains("value") && !j["value"].is_null()) {
		tv.value = j["value"].get<double>();
	}
}

void from_json(const nlohmann::json& j, GridpointLayer& layer) {
	layer.uom = json_string(j, "uom");
	layer.unit = parse_unit_code(layer.uom);

	if (j.contains("values") && j["values"].is_array()) {
		layer.values.clear();
		for (const auto& v : j["values"]) {
			GridpointTimeValue tv;
			from_json(v, tv);
			layer.values.push_back(std::move(tv));
		}
	}
}

void from_json(const nlohmann::json& j, GridpointProperties& p) {
	p.update_time = json_string(j, "updateTime");
	p.valid_times = json_string(j, "validTimes");
	p.grid_id = json_string(j, "gridId");
	p.grid_x = json_int(j, "gridX");
	p.grid_y = json_int(j, "gridY");

	if (j.contains("elevation") && !j["elevation"].is_null()) {
		from_json(j["elevation"], p.elevation);
	}

	parse_layer(j, "temperature", p.temperature);
	parse_layer(j, "dewpoint", p.dewpoint);
	parse_layer(j, "maxTemperature", p.max_temperature);
	parse_layer(j, "minTemperature", p.min_temperature);
	parse_layer(j, "relativeHumidity", p.relative_humidity);
	parse_layer(j, "apparentTemperature", p.apparent_temperature);
	parse_layer(j, "heatIndex", p.heat_index);
	parse_layer(j, "windChill", p.wind_chill);
	parse_layer(j, "skyCover", p.sky_cover);
	parse_layer(j, "windDirection", p.wind_direction);
	parse_layer(j, "windSpeed", p.wind_speed);
	parse_layer(j, "windGust", p.wind_gust);
	parse_layer(j, "probabilityOfPrecipitation", p.probability_of_precipitation);
	parse_layer(j, "quantitativePrecipitation", p.quantitative_precipitation);
	parse_layer(j, "iceAccumulation", p.ice_accumulation);
	parse_layer(j, "snowfallAmount", p.snowfall_amount);
	parse_layer(j, "ceilingHeight", p.ceiling_height);
	parse_layer(j, "visibility", p.visibility);
	parse_layer(j, "pressure", p.pressure);
}

void from_json(const nlohmann::json& j, GridpointResponse& r) {
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
