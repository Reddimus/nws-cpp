#include "nws/nws.hpp"

#include <iostream>

int main() {
	nws::NWSClient::Config config;
	config.http.user_agent = "(nws-cpp-example, dev@example.com)";

	nws::NWSClient client(std::move(config));

	// Get active alerts for Kansas
	auto alerts = client.get_active_alerts_by_area("KS");
	if (!alerts) {
		std::cerr << "Error: " << alerts.error().message << "\n";
		return 1;
	}

	std::cout << "Active alerts for Kansas: " << alerts->features.size() << "\n\n";

	for (const auto& alert : alerts->features) {
		const auto& p = alert.properties;
		std::cout << "Event: " << p.event << "\n";
		std::cout << "Severity: " << nws::to_string(p.severity) << "\n";
		std::cout << "Urgency: " << nws::to_string(p.urgency) << "\n";
		std::cout << "Certainty: " << nws::to_string(p.certainty) << "\n";
		std::cout << "Area: " << p.area_desc << "\n";
		if (p.headline) {
			std::cout << "Headline: " << *p.headline << "\n";
		}
		std::cout << "Effective: " << p.effective << "\n";
		std::cout << "Expires: " << p.expires << "\n";
		std::cout << "\n";
	}

	if (alerts->features.empty()) {
		std::cout << "No active alerts. Check another state or try later.\n";
	}

	return 0;
}
