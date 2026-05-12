#pragma once

#include "nws/error.hpp"
#include "nws/geo.hpp"
#include "nws/units.hpp"

#include <optional>
#include <string>
#include <string_view>
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

[[nodiscard]] Result<void> deserialize_radar_station_feature(std::string_view body,
															 RadarStationFeature& out);
[[nodiscard]] Result<void>
deserialize_radar_station_collection(std::string_view body, RadarStationCollectionResponse& out);
[[nodiscard]] Result<void> deserialize_radar_server_properties(std::string_view body,
															   RadarServerProperties& out);

} // namespace nws
