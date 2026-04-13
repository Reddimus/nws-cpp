#pragma once

#include "nws/geo.hpp"
#include "nws/units.hpp"

#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>

namespace nws {

struct RelativeLocationProperties {
	std::string city;
	std::string state;
	QuantitativeValue distance;
	QuantitativeValue bearing;
};

struct RelativeLocation {
	Geometry geometry{nullptr};
	RelativeLocationProperties properties;
};

struct PointProperties {
	std::string id;
	std::string grid_id;
	std::int32_t grid_x{0};
	std::int32_t grid_y{0};
	std::string forecast_url;
	std::string forecast_hourly_url;
	std::string forecast_grid_data_url;
	std::string observation_stations_url;
	std::string forecast_zone_url;
	std::string county_url;
	std::string fire_weather_zone_url;
	std::string time_zone;
	std::string radar_station;
	RelativeLocation relative_location;
};

using PointResponse = GeoJsonFeature<PointProperties>;

void from_json(const nlohmann::json& j, PointProperties& p);
void from_json(const nlohmann::json& j, PointResponse& r);

} // namespace nws
