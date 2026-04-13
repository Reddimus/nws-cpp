#include "nws/models/aviation.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, SigmetProperties& p) {
	p.id = j.value("id", "");
	p.atsu = j.value("atsu", "");
	p.sequence = j.value("sequence", "");
	p.phenomenon = j.value("phenomenon", "");
	p.valid_time_from = j.value("validTimeFrom", "");
	p.valid_time_to = j.value("validTimeTo", "");
}

void from_json(const nlohmann::json& j, SigmetFeature& r) {
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

void from_json(const nlohmann::json& j, SigmetCollectionResponse& r) {
	r.type = j.value("type", "FeatureCollection");
	if (j.contains("features") && j["features"].is_array()) {
		for (const auto& feat : j["features"]) {
			SigmetFeature sigmet;
			from_json(feat, sigmet);
			r.features.push_back(std::move(sigmet));
		}
	}
}

void from_json(const nlohmann::json& j, CWAProperties& p) {
	p.id = j.value("id", "");
	p.cwsu = j.value("cwsu", "");
	p.sequence = j.value("sequence", "");
	p.text = j.value("text", "");
	p.valid_time_from = j.value("validTimeFrom", "");
	p.valid_time_to = j.value("validTimeTo", "");
}

void from_json(const nlohmann::json& j, CWAFeature& r) {
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

void from_json(const nlohmann::json& j, CWACollectionResponse& r) {
	r.type = j.value("type", "FeatureCollection");
	if (j.contains("features") && j["features"].is_array()) {
		for (const auto& feat : j["features"]) {
			CWAFeature cwa;
			from_json(feat, cwa);
			r.features.push_back(std::move(cwa));
		}
	}
}

} // namespace nws
