// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
//
// Glaze-deserializer tests — verify the migration's parse output matches
// the documented NWS response shape and the pre-migration behavior
// (null-safe strings/ints, optional fields, polymorphic geometry,
// QuantitativeValue.unit derived from unit_code, mixed windSpeed
// string/object form on ForecastPeriod, etc.).

#include "nws/models/alert.hpp"
#include "nws/models/aviation.hpp"
#include "nws/models/common.hpp"
#include "nws/models/forecast.hpp"
#include "nws/models/glossary.hpp"
#include "nws/models/gridpoint.hpp"
#include "nws/models/observation.hpp"
#include "nws/models/office.hpp"
#include "nws/models/point.hpp"
#include "nws/models/product.hpp"
#include "nws/models/radar.hpp"
#include "nws/models/station.hpp"
#include "nws/models/zone.hpp"

#include <gtest/gtest.h>
#include <string>
#include <variant>

namespace nws {
namespace {

// ===== QuantitativeValue =====

TEST(GlazeDeserializerTest, QuantitativeValueDerivesUnitFromCode) {
	const std::string body = R"({"unitCode":"wmoUnit:degC","value":12.5})";
	QuantitativeValue qv;
	Result<void> r = deserialize_quantitative_value(body, qv);
	ASSERT_TRUE(r.has_value());
	EXPECT_EQ(qv.unit, Unit::DegC);
	ASSERT_TRUE(qv.value.has_value());
	EXPECT_DOUBLE_EQ(*qv.value, 12.5);
}

TEST(GlazeDeserializerTest, RejectsInvalidJson) {
	const std::string body = R"({"unitCode": "this is malformed)";
	QuantitativeValue qv;
	Result<void> r = deserialize_quantitative_value(body, qv);
	EXPECT_FALSE(r.has_value());
}

// ===== Point + GeoJSON geometry variant =====

TEST(GlazeDeserializerTest, PointResponseWithPolygonGeometry) {
	const std::string body = R"({
		"id": "https://example/points/1",
		"type": "Feature",
		"geometry": {
			"type": "Polygon",
			"coordinates": [[[-97.11, 39.72], [-97.10, 39.73], [-97.11, 39.72]]]
		},
		"properties": {
			"@id": "https://example/points/1",
			"gridId": "TOP",
			"gridX": 32,
			"gridY": 81,
			"timeZone": "America/Chicago",
			"radarStation": "KTWX"
		}
	})";
	PointResponse resp;
	Result<void> r = deserialize_point_response(body, resp);
	ASSERT_TRUE(r.has_value());
	EXPECT_EQ(resp.properties.grid_id, "TOP");
	EXPECT_EQ(resp.properties.grid_x, 32);
	ASSERT_TRUE(std::holds_alternative<GeoPolygon>(resp.geometry));
	const GeoPolygon& poly = std::get<GeoPolygon>(resp.geometry);
	ASSERT_EQ(poly.coordinates.size(), 1u);
	ASSERT_EQ(poly.coordinates[0].size(), 3u);
	EXPECT_NEAR(poly.coordinates[0][0].longitude, -97.11, 0.001);
}

TEST(GlazeDeserializerTest, PointResponseWithNullGeometry) {
	const std::string body = R"({
		"id": "x", "type": "Feature", "geometry": null,
		"properties": {"gridId": "TOP"}
	})";
	PointResponse resp;
	Result<void> r = deserialize_point_response(body, resp);
	ASSERT_TRUE(r.has_value());
	EXPECT_TRUE(std::holds_alternative<std::nullptr_t>(resp.geometry));
}

// ===== Forecast =====

TEST(GlazeDeserializerTest, ForecastPeriodWindSpeedAsString) {
	const std::string body = R"({
		"type": "Feature",
		"properties": {
			"periods": [{
				"number": 1, "name": "Tonight",
				"startTime": "2026-05-11T20:00:00-05:00",
				"endTime": "2026-05-12T06:00:00-05:00",
				"isDaytime": false, "temperature": 59,
				"temperatureUnit": "F", "windSpeed": "10 to 15 mph",
				"windDirection": "S", "icon": "x",
				"shortForecast": "Clear", "detailedForecast": "Mostly clear."
			}]
		}
	})";
	ForecastResponse f;
	Result<void> r = deserialize_forecast_response(body, f);
	ASSERT_TRUE(r.has_value());
	ASSERT_EQ(f.properties.periods.size(), 1u);
	EXPECT_EQ(f.properties.periods[0].wind_speed, "10 to 15 mph");
}

TEST(GlazeDeserializerTest, ForecastPeriodWindSpeedAsObject) {
	const std::string body = R"({
		"type": "Feature",
		"properties": {
			"periods": [{
				"number": 1, "name": "Today",
				"windSpeed": {"unitCode": "wmoUnit:km_h-1", "value": 18}
			}]
		}
	})";
	ForecastResponse f;
	Result<void> r = deserialize_forecast_response(body, f);
	ASSERT_TRUE(r.has_value());
	ASSERT_EQ(f.properties.periods.size(), 1u);
	// The pre-migration code rendered the {value, unitCode} form as
	// "18 wmoUnit:km_h-1"; preserve that for parity.
	EXPECT_EQ(f.properties.periods[0].wind_speed, "18 wmoUnit:km_h-1");
}

// ===== Gridpoint =====

TEST(GlazeDeserializerTest, GridpointResponseLayerExtraction) {
	const std::string body = R"({
		"type": "Feature",
		"geometry": null,
		"properties": {
			"updateTime": "2026-05-11T00:00:00+00:00",
			"validTimes": "2026-05-11T00:00:00+00:00/P7DT12H",
			"gridId": "TOP", "gridX": 32, "gridY": 81,
			"elevation": {"unitCode": "wmoUnit:m", "value": 456},
			"temperature": {
				"uom": "wmoUnit:degC",
				"values": [
					{"validTime": "2026-05-11T00:00:00+00:00/PT1H", "value": 12.5},
					{"validTime": "2026-05-11T01:00:00+00:00/PT1H", "value": null}
				]
			}
		}
	})";
	GridpointResponse g;
	Result<void> r = deserialize_gridpoint_response(body, g);
	ASSERT_TRUE(r.has_value());
	EXPECT_EQ(g.properties.grid_id, "TOP");
	EXPECT_DOUBLE_EQ(*g.properties.elevation.value, 456.0);
	ASSERT_TRUE(g.properties.temperature.has_value());
	EXPECT_EQ(g.properties.temperature->uom, "wmoUnit:degC");
	EXPECT_EQ(g.properties.temperature->unit, Unit::DegC);
	ASSERT_EQ(g.properties.temperature->values.size(), 2u);
	ASSERT_TRUE(g.properties.temperature->values[0].value.has_value());
	EXPECT_DOUBLE_EQ(*g.properties.temperature->values[0].value, 12.5);
	EXPECT_FALSE(g.properties.temperature->values[1].value.has_value());
	EXPECT_FALSE(g.properties.dewpoint.has_value()); // not present
}

// ===== Alert active count (dynamic-keyed maps) =====

TEST(GlazeDeserializerTest, AlertActiveCountMaps) {
	const std::string body = R"({
		"total": 12, "land": 10, "marine": 2,
		"regions": {"east": 5, "west": 7},
		"areas": {"TX": 3, "CA": 9},
		"zones": {}
	})";
	AlertActiveCount c;
	Result<void> r = deserialize_alert_active_count(body, c);
	ASSERT_TRUE(r.has_value());
	EXPECT_EQ(c.total, 12);
	EXPECT_EQ(c.land, 10);
	EXPECT_EQ(c.marine, 2);
	EXPECT_EQ(c.regions["east"], 5);
	EXPECT_EQ(c.regions["west"], 7);
	EXPECT_EQ(c.areas["TX"], 3);
	EXPECT_EQ(c.areas["CA"], 9);
	EXPECT_TRUE(c.zones.empty());
}

// ===== Alert collection =====

TEST(GlazeDeserializerTest, AlertCollectionMultipleFeatures) {
	const std::string body = R"({
		"type": "FeatureCollection",
		"features": [
			{"id": "a", "type": "Feature",
			 "properties": {"event": "Flood Warning", "severity": "Severe"}},
			{"id": "b", "type": "Feature",
			 "properties": {"event": "Wind Advisory", "severity": "Minor"}}
		]
	})";
	AlertCollectionResponse coll;
	Result<void> r = deserialize_alert_collection(body, coll);
	ASSERT_TRUE(r.has_value());
	ASSERT_EQ(coll.features.size(), 2u);
	EXPECT_EQ(coll.features[0].properties.event, "Flood Warning");
	EXPECT_EQ(coll.features[0].properties.severity, AlertSeverity::Severe);
	EXPECT_EQ(coll.features[1].properties.event, "Wind Advisory");
}

// ===== Observation + cloud layers =====

TEST(GlazeDeserializerTest, ObservationCloudLayers) {
	const std::string body = R"({
		"type": "Feature",
		"properties": {
			"stationId": "KTOP",
			"timestamp": "2026-05-11T00:45:00+00:00",
			"textDescription": "Cloudy",
			"cloudLayers": [
				{"base": {"unitCode": "wmoUnit:m", "value": 1500}, "amount": "OVC"},
				{"base": {"unitCode": "wmoUnit:m", "value": null}, "amount": "BKN"}
			]
		}
	})";
	ObservationResponse o;
	Result<void> r = deserialize_observation_response(body, o);
	ASSERT_TRUE(r.has_value());
	ASSERT_EQ(o.properties.cloud_layers.size(), 2u);
	EXPECT_EQ(o.properties.cloud_layers[0].amount, "OVC");
	ASSERT_TRUE(o.properties.cloud_layers[0].base.value.has_value());
	EXPECT_DOUBLE_EQ(*o.properties.cloud_layers[0].base.value, 1500.0);
	EXPECT_FALSE(o.properties.cloud_layers[1].base.value.has_value());
}

// ===== Glossary, office, product, aviation, radar, zone smoke =====

TEST(GlazeDeserializerTest, GlossaryParse) {
	const std::string body = R"({
		"glossary": [
			{"term": "MSLP", "definition": "Mean sea level pressure"},
			{"term": "TAF", "definition": "Terminal Area Forecast"}
		]
	})";
	GlossaryResponse g;
	Result<void> r = deserialize_glossary_response(body, g);
	ASSERT_TRUE(r.has_value());
	ASSERT_EQ(g.glossary.size(), 2u);
	EXPECT_EQ(g.glossary[1].term, "TAF");
}

TEST(GlazeDeserializerTest, ZoneForecastUnwrapsProperties) {
	const std::string body = R"({
		"properties": {
			"zone": ["KSZ001"],
			"periods": [{"number": 1, "name": "Tonight",
						 "detailedForecast": "Mostly clear."}]
		}
	})";
	ZoneForecastProperties z;
	Result<void> r = deserialize_zone_forecast(body, z);
	ASSERT_TRUE(r.has_value());
	ASSERT_EQ(z.zone.size(), 1u);
	EXPECT_EQ(z.zone[0], "KSZ001");
	ASSERT_EQ(z.periods.size(), 1u);
	EXPECT_EQ(z.periods[0].name, "Tonight");
}

TEST(GlazeDeserializerTest, RadarStationFallsBackToGeometryCoords) {
	const std::string body = R"({
		"id": "x",
		"type": "Feature",
		"geometry": {"type": "Point", "coordinates": [-97.5, 39.5]},
		"properties": {"id": "KTOP", "name": "Topeka", "stationType": "WSR-88D"}
	})";
	RadarStationFeature r;
	Result<void> rr = deserialize_radar_station_feature(body, r);
	ASSERT_TRUE(rr.has_value());
	EXPECT_NEAR(r.properties.longitude, -97.5, 0.001);
	EXPECT_NEAR(r.properties.latitude, 39.5, 0.001);
	EXPECT_EQ(r.properties.station_type, "WSR-88D");
}

TEST(GlazeDeserializerTest, OfficePropertiesSmoke) {
	const std::string body =
		R"({"id":"TOP","name":"Topeka","address":"1 NWS Way","email":"top@noaa.gov",
			"responsibleCounties":["A","B","C"]})";
	OfficeProperties o;
	Result<void> r = deserialize_office_properties(body, o);
	ASSERT_TRUE(r.has_value());
	EXPECT_EQ(o.id, "TOP");
	EXPECT_EQ(o.email, "top@noaa.gov");
	ASSERT_EQ(o.responsible_counties.size(), 3u);
}

TEST(GlazeDeserializerTest, ProductPropertiesSmoke) {
	const std::string body =
		R"({"id":"p1","issuingOffice":"TOP","productCode":"AFD","productName":"Area Forecast"})";
	ProductProperties p;
	Result<void> r = deserialize_product_properties(body, p);
	ASSERT_TRUE(r.has_value());
	EXPECT_EQ(p.product_code, "AFD");
	EXPECT_EQ(p.product_name, "Area Forecast");
}

TEST(GlazeDeserializerTest, SigmetCollectionSmoke) {
	const std::string body = R"({
		"type": "FeatureCollection",
		"features": [{"type": "Feature",
					  "properties": {"id": "s1", "phenomenon": "TURB"}}]
	})";
	SigmetCollectionResponse s;
	Result<void> r = deserialize_sigmet_collection(body, s);
	ASSERT_TRUE(r.has_value());
	ASSERT_EQ(s.features.size(), 1u);
	EXPECT_EQ(s.features[0].properties.phenomenon, "TURB");
}

} // namespace
} // namespace nws
