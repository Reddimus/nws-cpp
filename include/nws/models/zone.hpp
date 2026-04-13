#pragma once

#include "nws/geo.hpp"

#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <vector>

namespace nws {

/// Properties for a forecast zone
struct ZoneProperties {
	std::string id;
	std::string type;
	std::string name;
	std::string state;
	std::vector<std::string> forecast_offices;
	std::vector<std::string> observation_stations;
};

using ZoneFeature = GeoJsonFeature<ZoneProperties>;
using ZoneCollectionResponse = GeoJsonFeatureCollection<ZoneProperties>;

/// A single period in a zone forecast
struct ZoneForecastPeriod {
	std::int32_t number{0};
	std::string name;
	std::string detailed_forecast;
};

/// Properties for a zone forecast response
struct ZoneForecastProperties {
	std::vector<std::string> zone;
	std::vector<ZoneForecastPeriod> periods;
};

void from_json(const nlohmann::json& j, ZoneProperties& p);
void from_json(const nlohmann::json& j, ZoneFeature& r);
void from_json(const nlohmann::json& j, ZoneCollectionResponse& r);
void from_json(const nlohmann::json& j, ZoneForecastPeriod& p);
void from_json(const nlohmann::json& j, ZoneForecastProperties& p);

} // namespace nws
