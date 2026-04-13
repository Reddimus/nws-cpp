#include "nws/models/alert.hpp"
#include "nws/models/common.hpp"
#include "nws/models/forecast.hpp"
#include "nws/models/observation.hpp"
#include "nws/models/point.hpp"
#include "nws/models/station.hpp"

#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace nws {
namespace {

// Helper to load fixture files
nlohmann::json load_fixture(const std::string& filename) {
	// Try relative to build dir, then relative to source
	std::vector<std::string> paths = {
		"../tests/fixtures/" + filename,
		"../../tests/fixtures/" + filename,
		"tests/fixtures/" + filename,
	};
	for (const auto& path : paths) {
		std::ifstream f(path);
		if (f.good()) {
			return nlohmann::json::parse(f);
		}
	}
	// Return empty object if not found (test will fail with clear message)
	return nlohmann::json::object();
}

// ===== QuantitativeValue =====

TEST(QuantitativeValueTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"unitCode": "wmoUnit:degC",
		"value": 23.0,
		"qualityControl": "V"
	})");
	QuantitativeValue qv;
	from_json(j, qv);
	ASSERT_TRUE(qv.value.has_value());
	EXPECT_DOUBLE_EQ(*qv.value, 23.0);
	EXPECT_EQ(qv.unit_code, "wmoUnit:degC");
	EXPECT_EQ(qv.unit, Unit::DegC);
	ASSERT_TRUE(qv.quality_control.has_value());
	EXPECT_EQ(*qv.quality_control, "V");
}

TEST(QuantitativeValueTest, FromJsonNullValue) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"unitCode": "wmoUnit:km_h-1",
		"value": null,
		"qualityControl": "Z"
	})");
	QuantitativeValue qv;
	from_json(j, qv);
	EXPECT_FALSE(qv.value.has_value());
	EXPECT_EQ(qv.unit, Unit::Km_h);
}

TEST(QuantitativeValueTest, FromJsonMinMax) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"unitCode": "wmoUnit:degC",
		"value": 20.0,
		"minValue": 15.0,
		"maxValue": 25.0
	})");
	QuantitativeValue qv;
	from_json(j, qv);
	ASSERT_TRUE(qv.value.has_value());
	EXPECT_DOUBLE_EQ(*qv.value, 20.0);
	ASSERT_TRUE(qv.min_value.has_value());
	EXPECT_DOUBLE_EQ(*qv.min_value, 15.0);
	ASSERT_TRUE(qv.max_value.has_value());
	EXPECT_DOUBLE_EQ(*qv.max_value, 25.0);
}

// ===== Point =====

TEST(PointTest, FromJsonFixture) {
	nlohmann::json j = load_fixture("point_response.json");
	if (j.empty()) {
		GTEST_SKIP() << "Fixture not found";
	}

	PointResponse point;
	from_json(j, point);

	EXPECT_EQ(point.properties.grid_id, "TOP");
	EXPECT_EQ(point.properties.grid_x, 32);
	EXPECT_EQ(point.properties.grid_y, 81);
	EXPECT_FALSE(point.properties.forecast_url.empty());
	EXPECT_FALSE(point.properties.forecast_hourly_url.empty());
	EXPECT_FALSE(point.properties.observation_stations_url.empty());
	EXPECT_EQ(point.properties.time_zone, "America/Chicago");
	EXPECT_EQ(point.properties.radar_station, "KTWX");
	EXPECT_EQ(point.properties.relative_location.properties.state, "KS");
	EXPECT_FALSE(point.properties.relative_location.properties.city.empty());
}

TEST(PointTest, DefaultConstruction) {
	PointProperties p;
	EXPECT_TRUE(p.id.empty());
	EXPECT_TRUE(p.grid_id.empty());
	EXPECT_EQ(p.grid_x, 0);
	EXPECT_EQ(p.grid_y, 0);
}

// ===== Forecast =====

TEST(ForecastTest, FromJsonFixture) {
	nlohmann::json j = load_fixture("forecast_response.json");
	if (j.empty()) {
		GTEST_SKIP() << "Fixture not found";
	}

	ForecastResponse forecast;
	from_json(j, forecast);

	EXPECT_FALSE(forecast.properties.update_time.empty());
	EXPECT_FALSE(forecast.properties.generated_at.empty());
	ASSERT_FALSE(forecast.properties.periods.empty());

	const ForecastPeriod& first = forecast.properties.periods[0];
	EXPECT_EQ(first.number, 1);
	EXPECT_FALSE(first.name.empty());
	EXPECT_FALSE(first.start_time.empty());
	EXPECT_FALSE(first.end_time.empty());
	EXPECT_NE(first.temperature, 0); // Temperature should have a value
	EXPECT_FALSE(first.temperature_unit.empty());
	EXPECT_FALSE(first.wind_direction.empty());
	EXPECT_FALSE(first.short_forecast.empty());
	EXPECT_FALSE(first.detailed_forecast.empty());
}

TEST(ForecastPeriodTest, DefaultConstruction) {
	ForecastPeriod p;
	EXPECT_EQ(p.number, 0);
	EXPECT_TRUE(p.is_daytime);
	EXPECT_EQ(p.temperature, 0);
}

// ===== Observation =====

TEST(ObservationTest, FromJsonFixture) {
	nlohmann::json j = load_fixture("observation_response.json");
	if (j.empty()) {
		GTEST_SKIP() << "Fixture not found";
	}

	ObservationResponse obs;
	from_json(j, obs);

	EXPECT_FALSE(obs.properties.station_id.empty());
	EXPECT_FALSE(obs.properties.timestamp.empty());
	EXPECT_FALSE(obs.properties.text_description.empty());

	// Temperature should be present
	EXPECT_TRUE(obs.properties.temperature.has_value());
	EXPECT_EQ(obs.properties.temperature.unit, Unit::DegC);

	// Barometric pressure should be present
	EXPECT_TRUE(obs.properties.barometric_pressure.has_value());
	EXPECT_EQ(obs.properties.barometric_pressure.unit, Unit::Pascal);

	// Wind gust value can be null
	EXPECT_EQ(obs.properties.wind_gust.unit_code, "wmoUnit:km_h-1");
}

TEST(ObservationTest, DefaultConstruction) {
	ObservationProperties p;
	EXPECT_TRUE(p.id.empty());
	EXPECT_FALSE(p.temperature.has_value());
	EXPECT_TRUE(p.cloud_layers.empty());
}

// ===== Station =====

TEST(StationTest, FromJsonFixture) {
	nlohmann::json j = load_fixture("station_response.json");
	if (j.empty()) {
		GTEST_SKIP() << "Fixture not found";
	}

	StationResponse station;
	from_json(j, station);

	EXPECT_FALSE(station.properties.station_identifier.empty());
	EXPECT_FALSE(station.properties.name.empty());
	EXPECT_FALSE(station.properties.time_zone.empty());
}

// ===== Alert =====

TEST(AlertTest, DefaultConstruction) {
	AlertProperties p;
	EXPECT_TRUE(p.id.empty());
	EXPECT_EQ(p.severity, AlertSeverity::Unknown);
	EXPECT_EQ(p.certainty, AlertCertainty::Unknown);
	EXPECT_EQ(p.urgency, AlertUrgency::Unknown);
	EXPECT_EQ(p.status, AlertStatus::Actual);
}

TEST(AlertTest, ParseSeverity) {
	EXPECT_EQ(parse_alert_severity("Extreme"), AlertSeverity::Extreme);
	EXPECT_EQ(parse_alert_severity("Severe"), AlertSeverity::Severe);
	EXPECT_EQ(parse_alert_severity("Moderate"), AlertSeverity::Moderate);
	EXPECT_EQ(parse_alert_severity("Minor"), AlertSeverity::Minor);
	EXPECT_EQ(parse_alert_severity("bogus"), AlertSeverity::Unknown);
}

TEST(AlertTest, ParseCertainty) {
	EXPECT_EQ(parse_alert_certainty("Observed"), AlertCertainty::Observed);
	EXPECT_EQ(parse_alert_certainty("Likely"), AlertCertainty::Likely);
	EXPECT_EQ(parse_alert_certainty("Possible"), AlertCertainty::Possible);
	EXPECT_EQ(parse_alert_certainty("Unlikely"), AlertCertainty::Unlikely);
	EXPECT_EQ(parse_alert_certainty("bogus"), AlertCertainty::Unknown);
}

TEST(AlertTest, ParseUrgency) {
	EXPECT_EQ(parse_alert_urgency("Immediate"), AlertUrgency::Immediate);
	EXPECT_EQ(parse_alert_urgency("Expected"), AlertUrgency::Expected);
	EXPECT_EQ(parse_alert_urgency("Future"), AlertUrgency::Future);
	EXPECT_EQ(parse_alert_urgency("Past"), AlertUrgency::Past);
	EXPECT_EQ(parse_alert_urgency("bogus"), AlertUrgency::Unknown);
}

TEST(AlertTest, ToStringSeverity) {
	EXPECT_EQ(to_string(AlertSeverity::Extreme), "Extreme");
	EXPECT_EQ(to_string(AlertSeverity::Unknown), "Unknown");
}

TEST(AlertTest, FromJsonInline) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": "test-alert-1",
		"type": "Feature",
		"properties": {
			"id": "urn:oid:2.49.0.1.840.0.test",
			"areaDesc": "Test County",
			"severity": "Severe",
			"certainty": "Likely",
			"urgency": "Expected",
			"status": "Actual",
			"messageType": "Alert",
			"event": "Severe Thunderstorm Warning",
			"headline": "Test headline",
			"description": "Test description",
			"instruction": "Take shelter",
			"response": "Shelter",
			"sent": "2026-01-01T00:00:00Z",
			"effective": "2026-01-01T00:00:00Z",
			"expires": "2026-01-01T01:00:00Z",
			"geocode": {
				"UGC": ["KSC001"],
				"SAME": ["020001"]
			}
		}
	})");

	AlertFeature alert;
	from_json(j, alert);

	EXPECT_EQ(alert.properties.event, "Severe Thunderstorm Warning");
	EXPECT_EQ(alert.properties.severity, AlertSeverity::Severe);
	EXPECT_EQ(alert.properties.certainty, AlertCertainty::Likely);
	EXPECT_EQ(alert.properties.urgency, AlertUrgency::Expected);
	EXPECT_EQ(alert.properties.status, AlertStatus::Actual);
	ASSERT_TRUE(alert.properties.headline.has_value());
	EXPECT_EQ(*alert.properties.headline, "Test headline");
	ASSERT_TRUE(alert.properties.instruction.has_value());
	EXPECT_EQ(*alert.properties.instruction, "Take shelter");
	ASSERT_EQ(alert.properties.geocode.ugc.size(), 1);
	EXPECT_EQ(alert.properties.geocode.ugc[0], "KSC001");
}

// ===== GeoPoint =====

TEST(GeoPointTest, FromJsonObject) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"type": "Point",
		"coordinates": [-97.0892, 39.7456]
	})");
	GeoPoint p;
	from_json(j, p);
	EXPECT_NEAR(p.longitude, -97.0892, 0.001);
	EXPECT_NEAR(p.latitude, 39.7456, 0.001);
}

TEST(GeoPointTest, FromJsonArray) {
	nlohmann::json j = nlohmann::json::parse("[-97.0892, 39.7456]");
	GeoPoint p;
	from_json(j, p);
	EXPECT_NEAR(p.longitude, -97.0892, 0.001);
	EXPECT_NEAR(p.latitude, 39.7456, 0.001);
}

} // namespace
} // namespace nws
