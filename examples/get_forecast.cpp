#include "nws/nws.hpp"

#include <iostream>

int main() {
	nws::NWSClient::Config config;
	config.http.user_agent = "(nws-cpp-example, dev@example.com)";
	config.enable_cache = true;

	nws::NWSClient client(std::move(config));

	// Get forecast for Austin, TX
	nws::Result<nws::ForecastResponse> forecast =
		client.get_forecast_for_location(30.2672, -97.7431);
	if (!forecast) {
		std::cerr << "Error: " << forecast.error().message << "\n";
		if (!forecast.error().detail.empty()) {
			std::cerr << "Detail: " << forecast.error().detail << "\n";
		}
		return 1;
	}

	std::cout << "Forecast generated: " << forecast->properties.generated_at << "\n\n";

	for (const auto& period : forecast->properties.periods) {
		std::cout << period.name << ": " << period.temperature << period.temperature_unit << "\n";
		std::cout << "  Wind: " << period.wind_speed << " " << period.wind_direction << "\n";
		std::cout << "  " << period.short_forecast << "\n";
		if (period.probability_of_precipitation.has_value()) {
			std::cout << "  Precip: " << *period.probability_of_precipitation.value << "%\n";
		}
		std::cout << "\n";
	}

	return 0;
}
