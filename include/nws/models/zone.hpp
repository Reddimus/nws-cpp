#pragma once

#include "nws/error.hpp"
#include "nws/geo.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
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

[[nodiscard]] Result<void> deserialize_zone_feature(std::string_view body, ZoneFeature& out);
[[nodiscard]] Result<void> deserialize_zone_collection(std::string_view body,
													   ZoneCollectionResponse& out);
[[nodiscard]] Result<void> deserialize_zone_forecast(std::string_view body,
													 ZoneForecastProperties& out);

} // namespace nws
