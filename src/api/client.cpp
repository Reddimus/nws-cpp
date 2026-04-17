#include "nws/api.hpp"
#include "nws/models/common.hpp"

#include <format>
#include <nlohmann/json.hpp>

namespace nws {

struct NWSClient::Impl {
	HttpClient http;
	RateLimiter rate_limiter;
	RetryPolicy retry_policy;
	std::optional<LruCache<CoordinateKey, PointProperties>> points_cache;

	Impl(Config config)
		: http(std::move(config.http)), rate_limiter(config.rate_limit),
		  retry_policy(config.retry) {
		if (config.enable_cache) {
			points_cache.emplace(config.cache);
		}
	}
};

NWSClient::NWSClient(Config config) : impl_(std::make_unique<Impl>(std::move(config))) {}
NWSClient::~NWSClient() = default;
NWSClient::NWSClient(NWSClient&&) noexcept = default;
NWSClient& NWSClient::operator=(NWSClient&&) noexcept = default;

HttpClient& NWSClient::http_client() {
	return impl_->http;
}
const HttpClient& NWSClient::http_client() const {
	return impl_->http;
}

Result<HttpResponse> NWSClient::do_get(std::string_view path) {
	(void)impl_->rate_limiter.acquire();
	return with_retry([&]() { return impl_->http.get(path); }, impl_->retry_policy);
}

namespace {

void append_param(std::string& query, bool& has_params, const std::string& key,
				  const std::string& value) {
	query += has_params ? "&" : "?";
	query += key + "=" + value;
	has_params = true;
}

void append_vector_param(std::string& query, bool& has_params, const std::string& key,
						 const std::vector<std::string>& values) {
	for (const auto& v : values) {
		append_param(query, has_params, key, v);
	}
}

// Extract correlation ID from response headers
std::string get_correlation_id(const HttpResponse& resp) {
	for (const auto& [k, v] : resp.headers) {
		if (k == "X-Correlation-Id" || k == "x-correlation-id") {
			return v;
		}
	}
	return {};
}

} // namespace

// ===== Points API =====

Result<PointResponse> NWSClient::get_point(double latitude, double longitude) {
	// Check cache first
	if (impl_->points_cache) {
		CoordinateKey key{latitude, longitude};
		std::optional<PointProperties> cached = impl_->points_cache->get(key);
		if (cached) {
			PointResponse resp;
			resp.properties = std::move(*cached);
			return resp;
		}
	}

	Result<HttpResponse> result = do_get(std::format("/points/{},{}", latitude, longitude));
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		PointResponse response;
		from_json(j, response);

		// Cache the result
		if (impl_->points_cache) {
			impl_->points_cache->put(CoordinateKey{latitude, longitude}, response.properties);
		}

		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

// ===== Gridpoints/Forecasts API =====

Result<ForecastResponse> NWSClient::get_forecast(const std::string& wfo, std::int32_t x,
												 std::int32_t y, std::optional<UnitSystem> units) {
	std::string path =
		"/gridpoints/" + wfo + "/" + std::to_string(x) + "," + std::to_string(y) + "/forecast";
	if (units) {
		path += (*units == UnitSystem::SI) ? "?units=si" : "?units=us";
	}

	Result<HttpResponse> result = do_get(path);
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		ForecastResponse response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

Result<ForecastResponse> NWSClient::get_forecast_hourly(const std::string& wfo, std::int32_t x,
														std::int32_t y,
														std::optional<UnitSystem> units) {
	std::string path = "/gridpoints/" + wfo + "/" + std::to_string(x) + "," + std::to_string(y) +
					   "/forecast/hourly";
	if (units) {
		path += (*units == UnitSystem::SI) ? "?units=si" : "?units=us";
	}

	Result<HttpResponse> result = do_get(path);
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		ForecastResponse response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

Result<StationCollectionResponse>
NWSClient::get_gridpoint_stations(const std::string& wfo, std::int32_t x, std::int32_t y) {
	std::string path =
		"/gridpoints/" + wfo + "/" + std::to_string(x) + "," + std::to_string(y) + "/stations";

	Result<HttpResponse> result = do_get(path);
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		StationCollectionResponse response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

// ===== Stations API =====

Result<StationResponse> NWSClient::get_station(const std::string& station_id) {
	Result<HttpResponse> result = do_get("/stations/" + station_id);
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		StationResponse response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

// ===== Observations API =====

std::string NWSClient::build_observations_query(const GetObservationsParams& params) {
	std::string query;
	bool has_params = false;
	if (params.start) {
		append_param(query, has_params, "start", *params.start);
	}
	if (params.end) {
		append_param(query, has_params, "end", *params.end);
	}
	if (params.limit) {
		append_param(query, has_params, "limit", std::to_string(*params.limit));
	}
	if (params.cursor) {
		append_param(query, has_params, "cursor", *params.cursor);
	}
	return query;
}

Result<ObservationCollectionResponse>
NWSClient::get_observations(const std::string& station_id, const GetObservationsParams& params) {
	std::string path = "/stations/" + station_id + "/observations";
	path += build_observations_query(params);

	Result<HttpResponse> result = do_get(path);
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		ObservationCollectionResponse response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

Result<ObservationResponse> NWSClient::get_latest_observation(const std::string& station_id) {
	Result<HttpResponse> result = do_get("/stations/" + station_id + "/observations/latest");
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		ObservationResponse response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

// ===== Alerts API =====

std::string NWSClient::build_alerts_query(const GetAlertsParams& params) {
	std::string query;
	bool has_params = false;

	if (params.status) {
		append_vector_param(query, has_params, "status", *params.status);
	}
	if (params.message_type) {
		append_param(query, has_params, "message_type", *params.message_type);
	}
	if (params.event) {
		append_param(query, has_params, "event", *params.event);
	}
	if (params.code) {
		append_param(query, has_params, "code", *params.code);
	}
	if (params.area) {
		append_vector_param(query, has_params, "area", *params.area);
	}
	if (params.point) {
		append_param(query, has_params, "point", *params.point);
	}
	if (params.region) {
		append_vector_param(query, has_params, "region", *params.region);
	}
	if (params.zone) {
		append_vector_param(query, has_params, "zone", *params.zone);
	}
	if (params.urgency) {
		append_param(query, has_params, "urgency", *params.urgency);
	}
	if (params.severity) {
		append_param(query, has_params, "severity", *params.severity);
	}
	if (params.certainty) {
		append_param(query, has_params, "certainty", *params.certainty);
	}
	if (params.limit) {
		append_param(query, has_params, "limit", std::to_string(*params.limit));
	}
	if (params.cursor) {
		append_param(query, has_params, "cursor", *params.cursor);
	}
	return query;
}

Result<AlertCollectionResponse> NWSClient::get_alerts(const GetAlertsParams& params) {
	std::string path = "/alerts" + build_alerts_query(params);

	Result<HttpResponse> result = do_get(path);
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		AlertCollectionResponse response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

Result<AlertCollectionResponse> NWSClient::get_active_alerts(const GetAlertsParams& params) {
	std::string path = "/alerts/active" + build_alerts_query(params);

	Result<HttpResponse> result = do_get(path);
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		AlertCollectionResponse response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

Result<AlertActiveCount> NWSClient::get_active_alert_count() {
	Result<HttpResponse> result = do_get("/alerts/active/count");
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		AlertActiveCount response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

Result<AlertCollectionResponse> NWSClient::get_active_alerts_by_area(const std::string& area) {
	Result<HttpResponse> result = do_get("/alerts/active/area/" + area);
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		AlertCollectionResponse response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

Result<AlertCollectionResponse> NWSClient::get_active_alerts_by_zone(const std::string& zone_id) {
	Result<HttpResponse> result = do_get("/alerts/active/zone/" + zone_id);
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		AlertCollectionResponse response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

Result<AlertFeature> NWSClient::get_alert(const std::string& id) {
	Result<HttpResponse> result = do_get("/alerts/" + id);
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		AlertFeature response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

Result<std::vector<std::string>> NWSClient::get_alert_types() {
	Result<HttpResponse> result = do_get("/alerts/types");
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		if (j.contains("eventTypes") && j["eventTypes"].is_array()) {
			return j["eventTypes"].get<std::vector<std::string>>();
		}
		return std::vector<std::string>{};
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

// ===== Gridpoints API =====

Result<GridpointResponse> NWSClient::get_gridpoint(const std::string& wfo, std::int32_t x,
												   std::int32_t y) {
	std::string path = "/gridpoints/" + wfo + "/" + std::to_string(x) + "," + std::to_string(y);

	Result<HttpResponse> result = do_get(path);
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		GridpointResponse response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

// ===== Zones API =====

Result<ZoneFeature> NWSClient::get_zone(const std::string& type, const std::string& zone_id) {
	Result<HttpResponse> result = do_get("/zones/" + type + "/" + zone_id);
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		ZoneFeature response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

Result<ZoneForecastProperties> NWSClient::get_zone_forecast(const std::string& type,
															const std::string& zone_id) {
	Result<HttpResponse> result = do_get("/zones/" + type + "/" + zone_id + "/forecast");
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		ZoneForecastProperties response;
		if (j.contains("properties") && !j["properties"].is_null()) {
			from_json(j["properties"], response);
		}
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

// ===== Glossary API =====

Result<GlossaryResponse> NWSClient::get_glossary() {
	Result<HttpResponse> result = do_get("/glossary");
	if (!result) {
		return std::unexpected(result.error());
	}
	if (result->status_code != 200) {
		return std::unexpected(
			Error::from_response(result->status_code, result->body, get_correlation_id(*result)));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(result->body);
		GlossaryResponse response;
		from_json(j, response);
		return response;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(e.what()));
	}
}

// ===== Convenience Methods =====

Result<ForecastResponse> NWSClient::get_forecast_for_location(double latitude, double longitude,
															  std::optional<UnitSystem> units) {
	Result<PointResponse> point = get_point(latitude, longitude);
	if (!point) {
		return std::unexpected(point.error());
	}

	return get_forecast(point->properties.grid_id, point->properties.grid_x,
						point->properties.grid_y, units);
}

Result<ObservationResponse> NWSClient::get_current_weather(double latitude, double longitude) {
	Result<PointResponse> point = get_point(latitude, longitude);
	if (!point) {
		return std::unexpected(point.error());
	}

	// Get nearest station
	Result<StationCollectionResponse> stations = get_gridpoint_stations(
		point->properties.grid_id, point->properties.grid_x, point->properties.grid_y);
	if (!stations) {
		return std::unexpected(stations.error());
	}

	if (stations->features.empty()) {
		return std::unexpected(Error::not_found("No observation stations found near location"));
	}

	// Get latest observation from nearest station
	const auto& nearest = stations->features[0];
	return get_latest_observation(nearest.properties.station_identifier);
}

} // namespace nws
