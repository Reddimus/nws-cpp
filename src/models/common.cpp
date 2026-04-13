#include "nws/models/common.hpp"

#include "nws/geo.hpp"
#include "nws/units.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, QuantitativeValue& qv) {
	if (j.is_null()) {
		return;
	}
	qv.unit_code =
		j.contains("unitCode") && j["unitCode"].is_string() ? j["unitCode"].get<std::string>() : "";
	qv.unit = parse_unit_code(qv.unit_code);

	if (j.contains("value") && !j["value"].is_null()) {
		qv.value = j["value"].get<double>();
	}
	if (j.contains("maxValue") && !j["maxValue"].is_null()) {
		qv.max_value = j["maxValue"].get<double>();
	}
	if (j.contains("minValue") && !j["minValue"].is_null()) {
		qv.min_value = j["minValue"].get<double>();
	}
	if (j.contains("qualityControl") && !j["qualityControl"].is_null()) {
		qv.quality_control = j["qualityControl"].get<std::string>();
	}
}

void from_json(const nlohmann::json& j, GeoPoint& p) {
	// GeoJSON coordinates are [longitude, latitude]
	if (j.is_object() && j.contains("coordinates")) {
		const auto& coords = j["coordinates"];
		if (coords.is_array() && coords.size() >= 2) {
			p.longitude = coords[0].get<double>();
			p.latitude = coords[1].get<double>();
		}
	} else if (j.is_array() && j.size() >= 2) {
		p.longitude = j[0].get<double>();
		p.latitude = j[1].get<double>();
	}
}

std::string json_string(const nlohmann::json& j, const char* key) {
	if (j.contains(key) && j[key].is_string()) {
		return j[key].get<std::string>();
	}
	return {};
}

std::int32_t json_int(const nlohmann::json& j, const char* key, std::int32_t def) {
	if (j.contains(key) && j[key].is_number()) {
		return j[key].get<std::int32_t>();
	}
	return def;
}

} // namespace nws
