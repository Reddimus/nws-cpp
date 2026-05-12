#pragma once

#include "nws/error.hpp"
#include "nws/geo.hpp"
#include "nws/units.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nws {

struct ForecastPeriod {
	// 4-byte fields
	std::int32_t number{0};

	// 1-byte fields
	bool is_daytime{true};

	// String/complex fields
	std::string name;
	std::string start_time;
	std::string end_time;
	std::int32_t temperature{0};
	std::string temperature_unit;
	std::optional<std::string> temperature_trend;
	QuantitativeValue probability_of_precipitation;
	std::string wind_speed;
	std::string wind_direction;
	std::string icon;
	std::string short_forecast;
	std::string detailed_forecast;
};

struct ForecastProperties {
	std::string update_time;
	std::string generated_at;
	QuantitativeValue elevation;
	std::vector<ForecastPeriod> periods;
};

using ForecastResponse = GeoJsonFeature<ForecastProperties>;

[[nodiscard]] Result<void> deserialize_forecast_response(std::string_view body,
														 ForecastResponse& out);

} // namespace nws
