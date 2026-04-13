#include "nws/nws.hpp"

#include <iostream>

int main() {
	nws::NWSClient::Config config;
	config.http.user_agent = "(nws-cpp-example, dev@example.com)";

	nws::NWSClient client(std::move(config));

	// Get current weather for San Francisco
	auto obs = client.get_current_weather(37.7749, -122.4194);
	if (!obs) {
		std::cerr << "Error: " << obs.error().message << "\n";
		return 1;
	}

	const auto& p = obs->properties;
	std::cout << "Station: " << p.station_name << " (" << p.station_id << ")\n";
	std::cout << "Time: " << p.timestamp << "\n";
	std::cout << "Conditions: " << p.text_description << "\n\n";

	if (p.temperature.has_value()) {
		auto temp_f = p.temperature.converted_to(nws::Unit::DegF);
		std::cout << "Temperature: " << *p.temperature.value << " "
				  << nws::to_string(p.temperature.unit);
		if (temp_f.value.has_value()) {
			std::cout << " (" << static_cast<int>(*temp_f.value) << " F)";
		}
		std::cout << "\n";
	}

	if (p.dewpoint.has_value()) {
		std::cout << "Dewpoint: " << *p.dewpoint.value << " " << nws::to_string(p.dewpoint.unit)
				  << "\n";
	}

	if (p.wind_speed.has_value()) {
		auto wind_mph = p.wind_speed.converted_to(nws::Unit::Mi_h);
		std::cout << "Wind: " << *p.wind_speed.value << " " << nws::to_string(p.wind_speed.unit);
		if (wind_mph.value.has_value()) {
			std::cout << " (" << static_cast<int>(*wind_mph.value) << " mph)";
		}
		if (p.wind_direction.has_value()) {
			std::cout << " from " << static_cast<int>(*p.wind_direction.value) << " deg";
		}
		std::cout << "\n";
	}

	if (p.barometric_pressure.has_value()) {
		auto mb = p.barometric_pressure.converted_to(nws::Unit::Millibar);
		if (mb.value.has_value()) {
			std::cout << "Pressure: " << static_cast<int>(*mb.value) << " mbar\n";
		}
	}

	if (p.visibility.has_value()) {
		auto miles = p.visibility.converted_to(nws::Unit::Mile);
		if (miles.value.has_value()) {
			std::cout << "Visibility: " << static_cast<int>(*miles.value) << " mi\n";
		}
	}

	return 0;
}
