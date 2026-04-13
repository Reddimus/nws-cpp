#pragma once

#include "nws/geo.hpp"
#include "nws/units.hpp"

#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <vector>

namespace nws {

struct StationProperties {
	std::string id;
	std::string station_identifier;
	std::string name;
	std::string time_zone;
	std::string forecast_url;
	std::string county_url;
	std::string fire_weather_zone_url;
	QuantitativeValue elevation;
};

using StationResponse = GeoJsonFeature<StationProperties>;
using StationCollectionResponse = GeoJsonFeatureCollection<StationProperties>;

void from_json(const nlohmann::json& j, StationProperties& p);
void from_json(const nlohmann::json& j, StationResponse& r);
void from_json(const nlohmann::json& j, StationCollectionResponse& r);

} // namespace nws
