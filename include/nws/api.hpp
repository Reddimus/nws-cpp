#pragma once

/// @file api.hpp
/// @brief Complete REST API client for the National Weather Service

#include "nws/cache.hpp"
#include "nws/error.hpp"
#include "nws/http_client.hpp"
#include "nws/models/alert.hpp"
#include "nws/models/forecast.hpp"
#include "nws/models/observation.hpp"
#include "nws/models/point.hpp"
#include "nws/models/station.hpp"
#include "nws/pagination.hpp"
#include "nws/rate_limit.hpp"
#include "nws/retry.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace nws {

/// Forecast unit system preference
enum class UnitSystem : std::uint8_t { US, SI };

// ===== Request parameter structures =====

struct GetAlertsParams {
	std::optional<std::vector<std::string>> status;
	std::optional<std::string> message_type;
	std::optional<std::string> event;
	std::optional<std::string> code;
	std::optional<std::vector<std::string>> area;
	std::optional<std::string> point;
	std::optional<std::vector<std::string>> region;
	std::optional<std::vector<std::string>> zone;
	std::optional<std::string> urgency;
	std::optional<std::string> severity;
	std::optional<std::string> certainty;
	std::optional<std::int32_t> limit;
	std::optional<std::string> cursor;
};

struct GetObservationsParams {
	std::optional<std::string> start;
	std::optional<std::string> end;
	std::optional<std::int32_t> limit;
	std::optional<std::string> cursor;
};

struct GetStationsParams {
	std::optional<std::vector<std::string>> id;
	std::optional<std::vector<std::string>> state;
	std::optional<std::int32_t> limit;
	std::optional<std::string> cursor;
};

struct GetZonesParams {
	std::optional<std::vector<std::string>> id;
	std::optional<std::vector<std::string>> area;
	std::optional<std::vector<std::string>> region;
	std::optional<std::string> type;
	std::optional<std::string> point;
	std::optional<bool> include_geometry;
	std::optional<std::int32_t> limit;
	std::optional<std::string> effective;
};

struct GetProductsParams {
	std::optional<std::vector<std::string>> location;
	std::optional<std::string> start;
	std::optional<std::string> end;
	std::optional<std::vector<std::string>> office;
	std::optional<std::vector<std::string>> wmoid;
	std::optional<std::vector<std::string>> type;
	std::optional<std::int32_t> limit;
};

/// Complete National Weather Service REST API client
///
/// Provides typed methods for all NWS API endpoints.
/// Uses HttpClient for HTTP communication with built-in rate limiting and retry.
class NWSClient {
public:
	struct Config {
		ClientConfig http;
		RateLimiter::Config rate_limit{};
		RetryPolicy retry{};
		LruCache<CoordinateKey, PointProperties>::Config cache{};
		bool enable_cache{false};
	};

	explicit NWSClient(Config config);
	~NWSClient();

	NWSClient(NWSClient&&) noexcept;
	NWSClient& operator=(NWSClient&&) noexcept;

	// Non-copyable
	NWSClient(const NWSClient&) = delete;
	NWSClient& operator=(const NWSClient&) = delete;

	// ===== Points API =====

	/// Get point metadata (grid coordinates, forecast URLs) for a lat/lon
	[[nodiscard]] Result<PointResponse> get_point(double latitude, double longitude);

	// ===== Gridpoints/Forecasts API =====

	/// Get 12-hour period forecast
	[[nodiscard]] Result<ForecastResponse>
	get_forecast(const std::string& wfo, std::int32_t x, std::int32_t y,
				 std::optional<UnitSystem> units = std::nullopt);

	/// Get hourly forecast
	[[nodiscard]] Result<ForecastResponse>
	get_forecast_hourly(const std::string& wfo, std::int32_t x, std::int32_t y,
						std::optional<UnitSystem> units = std::nullopt);

	/// Get nearby observation stations for a grid point
	[[nodiscard]] Result<StationCollectionResponse>
	get_gridpoint_stations(const std::string& wfo, std::int32_t x, std::int32_t y);

	// ===== Stations API =====

	/// Get a single station by ID
	[[nodiscard]] Result<StationResponse> get_station(const std::string& station_id);

	// ===== Observations API =====

	/// Get observations for a station
	[[nodiscard]] Result<ObservationCollectionResponse>
	get_observations(const std::string& station_id, const GetObservationsParams& params = {});

	/// Get the latest observation from a station
	[[nodiscard]] Result<ObservationResponse> get_latest_observation(const std::string& station_id);

	// ===== Alerts API =====

	/// Get alerts with optional filters
	[[nodiscard]] Result<AlertCollectionResponse> get_alerts(const GetAlertsParams& params = {});

	/// Get active alerts with optional filters
	[[nodiscard]] Result<AlertCollectionResponse>
	get_active_alerts(const GetAlertsParams& params = {});

	/// Get count of active alerts by category
	[[nodiscard]] Result<AlertActiveCount> get_active_alert_count();

	/// Get active alerts for a specific area (state code)
	[[nodiscard]] Result<AlertCollectionResponse>
	get_active_alerts_by_area(const std::string& area);

	/// Get active alerts for a specific zone
	[[nodiscard]] Result<AlertCollectionResponse>
	get_active_alerts_by_zone(const std::string& zone_id);

	/// Get a single alert by ID
	[[nodiscard]] Result<AlertFeature> get_alert(const std::string& id);

	/// Get list of alert event types
	[[nodiscard]] Result<std::vector<std::string>> get_alert_types();

	// ===== Convenience Methods =====

	/// Get forecast for a lat/lon (auto-resolves point -> grid -> forecast)
	[[nodiscard]] Result<ForecastResponse>
	get_forecast_for_location(double latitude, double longitude,
							  std::optional<UnitSystem> units = std::nullopt);

	/// Get current weather for a lat/lon (auto-resolves point -> station -> observation)
	[[nodiscard]] Result<ObservationResponse> get_current_weather(double latitude,
																  double longitude);

	/// Access the underlying HTTP client
	[[nodiscard]] HttpClient& http_client();
	[[nodiscard]] const HttpClient& http_client() const;

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;

	// Internal helpers
	[[nodiscard]] Result<HttpResponse> do_get(std::string_view path);
	[[nodiscard]] std::string build_alerts_query(const GetAlertsParams& params);
	[[nodiscard]] std::string build_observations_query(const GetObservationsParams& params);
};

} // namespace nws
