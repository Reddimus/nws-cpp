#pragma once

#include "nws/geo.hpp"
#include "nws/units.hpp"

#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <vector>

namespace nws {

/// A single time-value pair in a gridpoint data layer
struct GridpointTimeValue {
	std::string valid_time;
	std::optional<double> value;
};

/// A layer of gridpoint data (e.g. temperature, wind speed)
struct GridpointLayer {
	std::string uom; // raw unit-of-measure string
	Unit unit{Unit::Unknown};
	std::vector<GridpointTimeValue> values;
};

/// Properties for /gridpoints/{wfo}/{x},{y} response
struct GridpointProperties {
	std::string update_time;
	std::string valid_times;
	QuantitativeValue elevation;
	std::string grid_id;
	std::int32_t grid_x{0};
	std::int32_t grid_y{0};

	// Gridpoint data layers (each optional because they may not be present)
	std::optional<GridpointLayer> temperature;
	std::optional<GridpointLayer> dewpoint;
	std::optional<GridpointLayer> max_temperature;
	std::optional<GridpointLayer> min_temperature;
	std::optional<GridpointLayer> relative_humidity;
	std::optional<GridpointLayer> apparent_temperature;
	std::optional<GridpointLayer> heat_index;
	std::optional<GridpointLayer> wind_chill;
	std::optional<GridpointLayer> sky_cover;
	std::optional<GridpointLayer> wind_direction;
	std::optional<GridpointLayer> wind_speed;
	std::optional<GridpointLayer> wind_gust;
	std::optional<GridpointLayer> probability_of_precipitation;
	std::optional<GridpointLayer> quantitative_precipitation;
	std::optional<GridpointLayer> ice_accumulation;
	std::optional<GridpointLayer> snowfall_amount;
	std::optional<GridpointLayer> ceiling_height;
	std::optional<GridpointLayer> visibility;
	std::optional<GridpointLayer> pressure;
};

using GridpointResponse = GeoJsonFeature<GridpointProperties>;

void from_json(const nlohmann::json& j, GridpointTimeValue& tv);
void from_json(const nlohmann::json& j, GridpointLayer& layer);
void from_json(const nlohmann::json& j, GridpointProperties& p);
void from_json(const nlohmann::json& j, GridpointResponse& r);

} // namespace nws
