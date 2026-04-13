#include "nws/nws.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
	nws::NWSClient::Config config;
	config.http.user_agent = "(nws-cpp-example, dev@example.com)";
	config.enable_cache = true;

	nws::NWSClient client(std::move(config));

	const std::string station = "KJFK"; // JFK Airport, New York
	const int poll_count = 3;
	const auto interval = std::chrono::seconds(60);

	std::cout << "Monitoring station " << station << " (" << poll_count << " observations, "
			  << interval.count() << "s interval)\n\n";

	for (int i = 0; i < poll_count; ++i) {
		auto obs = client.get_latest_observation(station);
		if (!obs) {
			std::cerr << "[" << (i + 1) << "] Error: " << obs.error().message << "\n";
		} else {
			const auto& p = obs->properties;
			std::cout << "[" << (i + 1) << "] " << p.timestamp << " - " << p.text_description;

			if (p.temperature.has_value()) {
				auto f = nws::celsius_to_fahrenheit(*p.temperature.value);
				std::cout << " | " << static_cast<int>(f) << "F";
			}
			if (p.wind_speed.has_value()) {
				auto mph = p.wind_speed.converted_to(nws::Unit::Mi_h);
				if (mph.value.has_value()) {
					std::cout << " | Wind " << static_cast<int>(*mph.value) << " mph";
				}
			}
			std::cout << "\n";
		}

		if (i < poll_count - 1) {
			std::this_thread::sleep_for(interval);
		}
	}

	std::cout << "\nDone.\n";
	return 0;
}
