#include "nws/models/radar.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, RadarStationProperties& p) {
	p.id = j.value("id", "");
	p.name = j.value("name", "");
	p.station_type = j.value("stationType", "");

	// Coordinates may come from GeoJSON geometry or inline fields
	if (j.contains("latitude") && !j["latitude"].is_null()) {
		p.latitude = j["latitude"].get<double>();
	}
	if (j.contains("longitude") && !j["longitude"].is_null()) {
		p.longitude = j["longitude"].get<double>();
	}

	if (j.contains("elevation") && !j["elevation"].is_null()) {
		from_json(j["elevation"], p.elevation);
	}
}

void from_json(const nlohmann::json& j, RadarStationFeature& r) {
	r.id = j.value("id", "");
	r.type = j.value("type", "Feature");

	if (j.contains("geometry") && !j["geometry"].is_null()) {
		GeoPoint gp;
		from_json(j["geometry"], gp);
		r.geometry = gp;

		// Populate lat/lon from geometry if properties don't have them
		if (j.contains("properties") && !j["properties"].is_null()) {
			auto& props = j["properties"];
			if (!props.contains("latitude") || props["latitude"].is_null()) {
				r.properties.latitude = gp.latitude;
				r.properties.longitude = gp.longitude;
			}
		}
	}

	if (j.contains("properties") && !j["properties"].is_null()) {
		from_json(j["properties"], r.properties);
	}
}

void from_json(const nlohmann::json& j, RadarStationCollectionResponse& r) {
	r.type = j.value("type", "FeatureCollection");
	if (j.contains("features") && j["features"].is_array()) {
		for (const auto& feat : j["features"]) {
			RadarStationFeature station;
			from_json(feat, station);
			r.features.push_back(std::move(station));
		}
	}
}

void from_json(const nlohmann::json& j, RadarServerProperties& p) {
	p.id = j.value("id", "");
	p.host = j.value("host", "");
}

} // namespace nws
