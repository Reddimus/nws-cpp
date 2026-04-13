#pragma once

#include "nws/geo.hpp"
#include "nws/units.hpp"

#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <vector>

namespace nws {

struct CloudLayer {
	QuantitativeValue base;
	std::string amount; // OVC, BKN, SCT, FEW, SKC, CLR, VV
};

struct ObservationProperties {
	std::string id;
	std::string station_url;
	std::string station_id;
	std::string station_name;
	std::string timestamp;
	std::string raw_message;
	std::string text_description;
	std::string icon;
	QuantitativeValue elevation;

	// Core measurements
	QuantitativeValue temperature;
	QuantitativeValue dewpoint;
	QuantitativeValue wind_direction;
	QuantitativeValue wind_speed;
	QuantitativeValue wind_gust;
	QuantitativeValue barometric_pressure;
	QuantitativeValue sea_level_pressure;
	QuantitativeValue visibility;
	QuantitativeValue relative_humidity;

	// Derived
	QuantitativeValue wind_chill;
	QuantitativeValue heat_index;

	// Precipitation
	QuantitativeValue precipitation_last_hour;
	QuantitativeValue precipitation_last_3_hours;
	QuantitativeValue precipitation_last_6_hours;

	// Extremes
	QuantitativeValue max_temperature_last_24_hours;
	QuantitativeValue min_temperature_last_24_hours;

	// Sky
	std::vector<CloudLayer> cloud_layers;
};

using ObservationResponse = GeoJsonFeature<ObservationProperties>;
using ObservationCollectionResponse = GeoJsonFeatureCollection<ObservationProperties>;

void from_json(const nlohmann::json& j, CloudLayer& cl);
void from_json(const nlohmann::json& j, ObservationProperties& p);
void from_json(const nlohmann::json& j, ObservationResponse& r);
void from_json(const nlohmann::json& j, ObservationCollectionResponse& r);

} // namespace nws
