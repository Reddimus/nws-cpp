#include "nws/nws.hpp"

#include <iostream>

int main() {
	// Configure the client
	nws::NWSClient::Config config;
	config.http.user_agent = "(nws-cpp-example, dev@example.com)";
	config.enable_cache = true;

	nws::NWSClient client(std::move(config));

	// Look up a point (Topeka, KS)
	auto point = client.get_point(39.7456, -97.0892);
	if (!point) {
		std::cerr << "Error: " << point.error().message << "\n";
		return 1;
	}

	std::cout << "Grid: " << point->properties.grid_id << " (" << point->properties.grid_x << ","
			  << point->properties.grid_y << ")\n";
	std::cout << "Time Zone: " << point->properties.time_zone << "\n";
	std::cout << "Radar: " << point->properties.radar_station << "\n";
	std::cout << "Near: " << point->properties.relative_location.properties.city << ", "
			  << point->properties.relative_location.properties.state << "\n";
	std::cout << "Forecast URL: " << point->properties.forecast_url << "\n";

	return 0;
}
