#include "nws/models/aviation.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, SigmetProperties& p) {
	p.id = json_string(j, "id");
	p.atsu = json_string(j, "atsu");
	p.sequence = json_string(j, "sequence");
	p.phenomenon = json_string(j, "phenomenon");
	p.valid_time_from = json_string(j, "validTimeFrom");
	p.valid_time_to = json_string(j, "validTimeTo");
}

void from_json(const nlohmann::json& j, SigmetFeature& r) {
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

void from_json(const nlohmann::json& j, SigmetCollectionResponse& r) {
	r.type = j.contains("type") && j["type"].is_string() ? j["type"].get<std::string>()
														 : "FeatureCollection";
	if (j.contains("features") && j["features"].is_array()) {
		for (const auto& feat : j["features"]) {
			SigmetFeature sigmet;
			from_json(feat, sigmet);
			r.features.push_back(std::move(sigmet));
		}
	}
}

void from_json(const nlohmann::json& j, CWAProperties& p) {
	p.id = json_string(j, "id");
	p.cwsu = json_string(j, "cwsu");
	p.sequence = json_string(j, "sequence");
	p.text = json_string(j, "text");
	p.valid_time_from = json_string(j, "validTimeFrom");
	p.valid_time_to = json_string(j, "validTimeTo");
}

void from_json(const nlohmann::json& j, CWAFeature& r) {
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

void from_json(const nlohmann::json& j, CWACollectionResponse& r) {
	r.type = j.contains("type") && j["type"].is_string() ? j["type"].get<std::string>()
														 : "FeatureCollection";
	if (j.contains("features") && j["features"].is_array()) {
		for (const auto& feat : j["features"]) {
			CWAFeature cwa;
			from_json(feat, cwa);
			r.features.push_back(std::move(cwa));
		}
	}
}

} // namespace nws
