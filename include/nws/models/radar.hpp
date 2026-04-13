#pragma once

#include "nws/geo.hpp"
#include "nws/units.hpp"

#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <vector>

namespace nws {

/// Properties for a radar station
struct RadarStationProperties {
	std::string id;
	std::string name;
	std::string station_type;
	double latitude{0.0};
	double longitude{0.0};
	QuantitativeValue elevation;
};

using RadarStationFeature = GeoJsonFeature<RadarStationProperties>;
using RadarStationCollectionResponse = GeoJsonFeatureCollection<RadarStationProperties>;

/// Properties for a radar server
struct RadarServerProperties {
	std::string id;
	std::string host;
};

void from_json(const nlohmann::json& j, RadarStationProperties& p);
void from_json(const nlohmann::json& j, RadarStationFeature& r);
void from_json(const nlohmann::json& j, RadarStationCollectionResponse& r);
void from_json(const nlohmann::json& j, RadarServerProperties& p);

} // namespace nws
