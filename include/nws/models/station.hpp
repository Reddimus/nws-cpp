#pragma once

#include "nws/error.hpp"
#include "nws/geo.hpp"
#include "nws/units.hpp"

#include <optional>
#include <string>
#include <string_view>
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

[[nodiscard]] Result<void> deserialize_station_response(std::string_view body,
														StationResponse& out);
[[nodiscard]] Result<void> deserialize_station_collection(std::string_view body,
														  StationCollectionResponse& out);

} // namespace nws
