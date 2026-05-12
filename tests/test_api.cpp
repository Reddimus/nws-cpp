#include "nws/api.hpp"
#include "nws/models/common.hpp"

#include <gtest/gtest.h>
#include <string>

namespace nws {
namespace {

// ===== Config defaults =====

TEST(NWSClientConfigTest, Defaults) {
	NWSClient::Config config;
	EXPECT_EQ(config.http.base_url, "https://api.weather.gov");
	EXPECT_EQ(config.http.accept, "application/geo+json");
	EXPECT_EQ(config.http.timeout, std::chrono::seconds(30));
	EXPECT_TRUE(config.http.verify_ssl);
	EXPECT_FALSE(config.enable_cache);
}

TEST(NWSClientConfigTest, RateLimitDefaults) {
	NWSClient::Config config;
	EXPECT_EQ(config.rate_limit.max_tokens, 5);
	EXPECT_EQ(config.rate_limit.refill_interval, std::chrono::milliseconds(1000));
}

TEST(NWSClientConfigTest, RetryDefaults) {
	NWSClient::Config config;
	EXPECT_EQ(config.retry.initial_delay, std::chrono::milliseconds(500));
	EXPECT_EQ(config.retry.max_delay, std::chrono::milliseconds(30000));
	EXPECT_EQ(config.retry.max_attempts, 3);
	EXPECT_TRUE(config.retry.retry_on_rate_limit);
}

// ===== UnitSystem =====

TEST(UnitSystemTest, DistinctValues) {
	EXPECT_NE(static_cast<int>(UnitSystem::US), static_cast<int>(UnitSystem::SI));
}

// ===== Params defaults =====

TEST(GetAlertsParamsTest, AllOptional) {
	GetAlertsParams params;
	EXPECT_FALSE(params.status.has_value());
	EXPECT_FALSE(params.event.has_value());
	EXPECT_FALSE(params.area.has_value());
	EXPECT_FALSE(params.severity.has_value());
	EXPECT_FALSE(params.limit.has_value());
	EXPECT_FALSE(params.cursor.has_value());
}

TEST(GetObservationsParamsTest, AllOptional) {
	GetObservationsParams params;
	EXPECT_FALSE(params.start.has_value());
	EXPECT_FALSE(params.end.has_value());
	EXPECT_FALSE(params.limit.has_value());
	EXPECT_FALSE(params.cursor.has_value());
}

TEST(GetStationsParamsTest, AllOptional) {
	GetStationsParams params;
	EXPECT_FALSE(params.id.has_value());
	EXPECT_FALSE(params.state.has_value());
	EXPECT_FALSE(params.limit.has_value());
}

TEST(GetZonesParamsTest, AllOptional) {
	GetZonesParams params;
	EXPECT_FALSE(params.type.has_value());
	EXPECT_FALSE(params.area.has_value());
	EXPECT_FALSE(params.limit.has_value());
}

TEST(GetProductsParamsTest, AllOptional) {
	GetProductsParams params;
	EXPECT_FALSE(params.location.has_value());
	EXPECT_FALSE(params.type.has_value());
	EXPECT_FALSE(params.limit.has_value());
}

// ===== Null-safety regression tests =====
//
// These exercise the pre-migration guarantee that every null-able JSON
// field decodes to a default value (empty string, default-constructed
// optional, etc.) without throwing or producing garbage. The post-Glaze
// pipeline preserves this via the detail::get_*() helpers, which treat
// missing / null / type-mismatched values uniformly.

TEST(NullSafetyTest, QuantitativeValueWithAllNulls) {
	const std::string body = R"({
		"unitCode": null,
		"value": null,
		"qualityControl": null
	})";
	QuantitativeValue qv;
	Result<void> r = deserialize_quantitative_value(body, qv);
	ASSERT_TRUE(r.has_value());
	EXPECT_FALSE(qv.has_value());
	EXPECT_TRUE(qv.unit_code.empty());
}

TEST(NullSafetyTest, PointPropertiesWithNullFields) {
	const std::string body = R"({
		"id": null,
		"type": "Feature",
		"properties": {
			"@id": null,
			"gridId": null,
			"gridX": null,
			"gridY": null,
			"forecast": null,
			"timeZone": null,
			"radarStation": null
		}
	})";
	PointResponse p;
	Result<void> r = deserialize_point_response(body, p);
	ASSERT_TRUE(r.has_value());
	EXPECT_TRUE(p.properties.grid_id.empty());
	EXPECT_EQ(p.properties.grid_x, 0);
}

TEST(NullSafetyTest, ForecastPeriodWithNullFields) {
	const std::string body = R"({
		"id": null,
		"type": "Feature",
		"properties": {
			"updateTime": null,
			"generatedAt": null,
			"periods": [{
				"number": null,
				"name": null,
				"startTime": null,
				"endTime": null,
				"isDaytime": null,
				"temperature": null,
				"temperatureUnit": null,
				"temperatureTrend": null,
				"windSpeed": null,
				"windDirection": null,
				"shortForecast": null,
				"detailedForecast": null
			}]
		}
	})";
	ForecastResponse f;
	Result<void> r = deserialize_forecast_response(body, f);
	ASSERT_TRUE(r.has_value());
	ASSERT_EQ(f.properties.periods.size(), 1u);
	EXPECT_TRUE(f.properties.periods[0].name.empty());
}

TEST(NullSafetyTest, ObservationWithNullMeasurements) {
	const std::string body = R"({
		"id": null,
		"type": "Feature",
		"properties": {
			"@id": null,
			"station": null,
			"stationId": null,
			"timestamp": null,
			"textDescription": null,
			"temperature": {"unitCode": "wmoUnit:degC", "value": null},
			"windSpeed": {"unitCode": "wmoUnit:km_h-1", "value": null}
		}
	})";
	ObservationResponse o;
	Result<void> r = deserialize_observation_response(body, o);
	ASSERT_TRUE(r.has_value());
	EXPECT_FALSE(o.properties.temperature.has_value());
	EXPECT_EQ(o.properties.temperature.unit, Unit::DegC);
}

TEST(NullSafetyTest, AlertWithNullFields) {
	const std::string body = R"({
		"id": null,
		"type": "Feature",
		"properties": {
			"id": null,
			"areaDesc": null,
			"severity": null,
			"certainty": null,
			"urgency": null,
			"status": null,
			"messageType": null,
			"event": null,
			"headline": null,
			"description": null,
			"instruction": null,
			"sent": null,
			"effective": null,
			"expires": null
		}
	})";
	AlertFeature a;
	Result<void> r = deserialize_alert_feature(body, a);
	ASSERT_TRUE(r.has_value());
	EXPECT_TRUE(a.properties.id.empty());
	EXPECT_EQ(a.properties.severity, AlertSeverity::Unknown);
}

TEST(NullSafetyTest, ErrorFromResponseWithNullJsonFields) {
	const std::string body = R"({"title": null, "detail": null, "correlationId": null})";
	Error err;
	EXPECT_NO_THROW(err = Error::from_response(500, body));
	EXPECT_EQ(err.code, ErrorCode::ServerError);
}

} // namespace
} // namespace nws
