#include "nws/api.hpp"
#include "nws/models/common.hpp"

#include <format>
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <string_view>
#include <utility>

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
	for (const std::string& v : values) {
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

// Common HTTP-result handling: status check + body/error extraction.
// On success returns the body string; on failure unpacks the http error
// into a parse-able Error. Used uniformly by every endpoint handler so
// the per-endpoint code path is a single deserialize_*() call.
Result<std::string> body_or_error(Result<HttpResponse>&& http_result) {
	if (!http_result) {
		return std::unexpected(http_result.error());
	}
	if (http_result->status_code != 200) {
		return std::unexpected(Error::from_response(http_result->status_code, http_result->body,
													get_correlation_id(*http_result)));
	}
	return std::move(http_result->body);
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

	Result<std::string> body =
		body_or_error(do_get(std::format("/points/{},{}", latitude, longitude)));
	if (!body) {
		return std::unexpected(body.error());
	}
	PointResponse response;
	Result<void> parse = deserialize_point_response(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}

	// Cache the result
	if (impl_->points_cache) {
		impl_->points_cache->put(CoordinateKey{latitude, longitude}, response.properties);
	}

	return response;
}

// ===== Gridpoints/Forecasts API =====

Result<ForecastResponse> NWSClient::get_forecast(const std::string& wfo, std::int32_t x,
												 std::int32_t y, std::optional<UnitSystem> units) {
	std::string path =
		"/gridpoints/" + wfo + "/" + std::to_string(x) + "," + std::to_string(y) + "/forecast";
	if (units) {
		path += (*units == UnitSystem::SI) ? "?units=si" : "?units=us";
	}

	Result<std::string> body = body_or_error(do_get(path));
	if (!body) {
		return std::unexpected(body.error());
	}
	ForecastResponse response;
	Result<void> parse = deserialize_forecast_response(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

Result<ForecastResponse> NWSClient::get_forecast_hourly(const std::string& wfo, std::int32_t x,
														std::int32_t y,
														std::optional<UnitSystem> units) {
	std::string path = "/gridpoints/" + wfo + "/" + std::to_string(x) + "," + std::to_string(y) +
					   "/forecast/hourly";
	if (units) {
		path += (*units == UnitSystem::SI) ? "?units=si" : "?units=us";
	}

	Result<std::string> body = body_or_error(do_get(path));
	if (!body) {
		return std::unexpected(body.error());
	}
	ForecastResponse response;
	Result<void> parse = deserialize_forecast_response(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

Result<StationCollectionResponse>
NWSClient::get_gridpoint_stations(const std::string& wfo, std::int32_t x, std::int32_t y) {
	std::string path =
		"/gridpoints/" + wfo + "/" + std::to_string(x) + "," + std::to_string(y) + "/stations";

	Result<std::string> body = body_or_error(do_get(path));
	if (!body) {
		return std::unexpected(body.error());
	}
	StationCollectionResponse response;
	Result<void> parse = deserialize_station_collection(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

// ===== Stations API =====

Result<StationResponse> NWSClient::get_station(const std::string& station_id) {
	Result<std::string> body = body_or_error(do_get("/stations/" + station_id));
	if (!body) {
		return std::unexpected(body.error());
	}
	StationResponse response;
	Result<void> parse = deserialize_station_response(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
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

	Result<std::string> body = body_or_error(do_get(path));
	if (!body) {
		return std::unexpected(body.error());
	}
	ObservationCollectionResponse response;
	Result<void> parse = deserialize_observation_collection(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

Result<ObservationResponse> NWSClient::get_latest_observation(const std::string& station_id) {
	Result<std::string> body =
		body_or_error(do_get("/stations/" + station_id + "/observations/latest"));
	if (!body) {
		return std::unexpected(body.error());
	}
	ObservationResponse response;
	Result<void> parse = deserialize_observation_response(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
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

	Result<std::string> body = body_or_error(do_get(path));
	if (!body) {
		return std::unexpected(body.error());
	}
	AlertCollectionResponse response;
	Result<void> parse = deserialize_alert_collection(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

Result<AlertCollectionResponse> NWSClient::get_active_alerts(const GetAlertsParams& params) {
	std::string path = "/alerts/active" + build_alerts_query(params);

	Result<std::string> body = body_or_error(do_get(path));
	if (!body) {
		return std::unexpected(body.error());
	}
	AlertCollectionResponse response;
	Result<void> parse = deserialize_alert_collection(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

Result<AlertActiveCount> NWSClient::get_active_alert_count() {
	Result<std::string> body = body_or_error(do_get("/alerts/active/count"));
	if (!body) {
		return std::unexpected(body.error());
	}
	AlertActiveCount response;
	Result<void> parse = deserialize_alert_active_count(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

Result<AlertCollectionResponse> NWSClient::get_active_alerts_by_area(const std::string& area) {
	Result<std::string> body = body_or_error(do_get("/alerts/active/area/" + area));
	if (!body) {
		return std::unexpected(body.error());
	}
	AlertCollectionResponse response;
	Result<void> parse = deserialize_alert_collection(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

Result<AlertCollectionResponse> NWSClient::get_active_alerts_by_zone(const std::string& zone_id) {
	Result<std::string> body = body_or_error(do_get("/alerts/active/zone/" + zone_id));
	if (!body) {
		return std::unexpected(body.error());
	}
	AlertCollectionResponse response;
	Result<void> parse = deserialize_alert_collection(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

Result<AlertFeature> NWSClient::get_alert(const std::string& id) {
	Result<std::string> body = body_or_error(do_get("/alerts/" + id));
	if (!body) {
		return std::unexpected(body.error());
	}
	AlertFeature response;
	Result<void> parse = deserialize_alert_feature(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

Result<std::vector<std::string>> NWSClient::get_alert_types() {
	Result<std::string> body = body_or_error(do_get("/alerts/types"));
	if (!body) {
		return std::unexpected(body.error());
	}
	// Small one-off shape: {"eventTypes": [...]}. Use glz::generic
	// directly here instead of carving out yet another deserialize_*.
	glz::generic root{};
	glz::error_ctx ec = glz::read_json(root, *body);
	if (ec) {
		return std::unexpected(Error::parse(glz::format_error(ec, *body)));
	}
	if (!root.is_object()) {
		return std::vector<std::string>{};
	}
	const glz::generic::object_t& obj = root.get_object();
	glz::generic::object_t::const_iterator it = obj.find("eventTypes");
	if (it == obj.end() || !it->second.is_array()) {
		return std::vector<std::string>{};
	}
	std::vector<std::string> out;
	const glz::generic::array_t& arr = it->second.get_array();
	out.reserve(arr.size());
	for (const glz::generic& v : arr) {
		if (v.is_string()) {
			out.push_back(v.get<std::string>());
		}
	}
	return out;
}

// ===== Gridpoints API =====

Result<GridpointResponse> NWSClient::get_gridpoint(const std::string& wfo, std::int32_t x,
												   std::int32_t y) {
	std::string path = "/gridpoints/" + wfo + "/" + std::to_string(x) + "," + std::to_string(y);

	Result<std::string> body = body_or_error(do_get(path));
	if (!body) {
		return std::unexpected(body.error());
	}
	GridpointResponse response;
	Result<void> parse = deserialize_gridpoint_response(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

// ===== Zones API =====

Result<ZoneFeature> NWSClient::get_zone(const std::string& type, const std::string& zone_id) {
	Result<std::string> body = body_or_error(do_get("/zones/" + type + "/" + zone_id));
	if (!body) {
		return std::unexpected(body.error());
	}
	ZoneFeature response;
	Result<void> parse = deserialize_zone_feature(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

Result<ZoneForecastProperties> NWSClient::get_zone_forecast(const std::string& type,
															const std::string& zone_id) {
	Result<std::string> body =
		body_or_error(do_get("/zones/" + type + "/" + zone_id + "/forecast"));
	if (!body) {
		return std::unexpected(body.error());
	}
	ZoneForecastProperties response;
	// deserialize_zone_forecast unwraps the {properties:{...}} envelope.
	Result<void> parse = deserialize_zone_forecast(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
}

// ===== Glossary API =====

Result<GlossaryResponse> NWSClient::get_glossary() {
	Result<std::string> body = body_or_error(do_get("/glossary"));
	if (!body) {
		return std::unexpected(body.error());
	}
	GlossaryResponse response;
	Result<void> parse = deserialize_glossary_response(*body, response);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return response;
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
