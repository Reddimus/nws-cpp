#include <nws/nws.hpp>

int main() {
	nws::NWSClient::Config config;
	config.http.user_agent = "nws-consumer-test";
	const nws::NWSClient client(config);
	nws::ForecastResponse response;
	return nws::deserialize_forecast_response(R"({"properties":{"periods":[]}})", response) &&
				   response.properties.periods.empty()
			   ? 0
			   : 1;
}
