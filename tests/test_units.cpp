#include "nws/units.hpp"

#include <cmath>
#include <gtest/gtest.h>

namespace nws {
namespace {

// Helper for floating point comparison
constexpr double kEpsilon = 0.01;

// ===== Unit parsing =====

TEST(ParseUnitCodeTest, Temperature) {
	EXPECT_EQ(parse_unit_code("wmoUnit:degC"), Unit::DegC);
	EXPECT_EQ(parse_unit_code("wmoUnit:degF"), Unit::DegF);
	EXPECT_EQ(parse_unit_code("wmoUnit:K"), Unit::Kelvin);
}

TEST(ParseUnitCodeTest, Speed) {
	EXPECT_EQ(parse_unit_code("wmoUnit:km_h-1"), Unit::Km_h);
	EXPECT_EQ(parse_unit_code("wmoUnit:m_s-1"), Unit::M_s);
	EXPECT_EQ(parse_unit_code("wmoUnit:kt"), Unit::Knot);
	EXPECT_EQ(parse_unit_code("wmoUnit:mi_h-1"), Unit::Mi_h);
}

TEST(ParseUnitCodeTest, Distance) {
	EXPECT_EQ(parse_unit_code("wmoUnit:m"), Unit::Meter);
	EXPECT_EQ(parse_unit_code("wmoUnit:km"), Unit::Kilometer);
	EXPECT_EQ(parse_unit_code("unit:mi"), Unit::Mile);
}

TEST(ParseUnitCodeTest, Pressure) {
	EXPECT_EQ(parse_unit_code("wmoUnit:Pa"), Unit::Pascal);
	EXPECT_EQ(parse_unit_code("wmoUnit:mbar"), Unit::Millibar);
	EXPECT_EQ(parse_unit_code("wmoUnit:hPa"), Unit::Millibar);
}

TEST(ParseUnitCodeTest, Precipitation) {
	EXPECT_EQ(parse_unit_code("wmoUnit:mm"), Unit::Millimeter);
	EXPECT_EQ(parse_unit_code("unit:in"), Unit::Inch);
	EXPECT_EQ(parse_unit_code("wmoUnit:cm"), Unit::Centimeter);
}

TEST(ParseUnitCodeTest, Other) {
	EXPECT_EQ(parse_unit_code("wmoUnit:degree_(angle)"), Unit::DegreeAngle);
	EXPECT_EQ(parse_unit_code("wmoUnit:percent"), Unit::Percent);
}

TEST(ParseUnitCodeTest, Unknown) {
	EXPECT_EQ(parse_unit_code(""), Unit::Unknown);
	EXPECT_EQ(parse_unit_code("bogus"), Unit::Unknown);
	EXPECT_EQ(parse_unit_code("wmoUnit:foo"), Unit::Unknown);
}

// ===== to_string =====

TEST(UnitToStringTest, AllUnits) {
	EXPECT_EQ(to_string(Unit::DegC), "C");
	EXPECT_EQ(to_string(Unit::DegF), "F");
	EXPECT_EQ(to_string(Unit::Mi_h), "mph");
	EXPECT_EQ(to_string(Unit::Pascal), "Pa");
	EXPECT_EQ(to_string(Unit::Percent), "%");
	EXPECT_EQ(to_string(Unit::Unknown), "?");
}

// ===== Temperature conversions =====

TEST(ConversionTest, CelsiusToFahrenheit) {
	EXPECT_NEAR(celsius_to_fahrenheit(0.0), 32.0, kEpsilon);
	EXPECT_NEAR(celsius_to_fahrenheit(100.0), 212.0, kEpsilon);
	EXPECT_NEAR(celsius_to_fahrenheit(-40.0), -40.0, kEpsilon);
	EXPECT_NEAR(celsius_to_fahrenheit(20.0), 68.0, kEpsilon);
}

TEST(ConversionTest, FahrenheitToCelsius) {
	EXPECT_NEAR(fahrenheit_to_celsius(32.0), 0.0, kEpsilon);
	EXPECT_NEAR(fahrenheit_to_celsius(212.0), 100.0, kEpsilon);
	EXPECT_NEAR(fahrenheit_to_celsius(-40.0), -40.0, kEpsilon);
}

// ===== Speed conversions =====

TEST(ConversionTest, KmhToMph) {
	EXPECT_NEAR(kmh_to_mph(100.0), 62.14, kEpsilon);
	EXPECT_NEAR(kmh_to_mph(0.0), 0.0, kEpsilon);
}

TEST(ConversionTest, MsToMph) {
	EXPECT_NEAR(ms_to_mph(1.0), 2.24, kEpsilon);
}

TEST(ConversionTest, KnotsToMph) {
	EXPECT_NEAR(knots_to_mph(1.0), 1.15, kEpsilon);
}

// ===== Pressure conversions =====

TEST(ConversionTest, PascalToMillibar) {
	EXPECT_NEAR(pascal_to_millibar(101325.0), 1013.25, kEpsilon);
	EXPECT_NEAR(pascal_to_millibar(100.0), 1.0, kEpsilon);
}

// ===== Length conversions =====

TEST(ConversionTest, MmToInches) {
	EXPECT_NEAR(mm_to_inches(25.4), 1.0, kEpsilon);
}

TEST(ConversionTest, MetersToMiles) {
	EXPECT_NEAR(meters_to_miles(1609.344), 1.0, kEpsilon);
}

TEST(ConversionTest, KmToMiles) {
	EXPECT_NEAR(km_to_miles(1.60934), 1.0, kEpsilon);
}

// ===== Generic convert function =====

TEST(ConvertTest, SameUnit) {
	auto result = convert(42.0, Unit::DegC, Unit::DegC);
	ASSERT_TRUE(result.has_value());
	EXPECT_NEAR(*result, 42.0, kEpsilon);
}

TEST(ConvertTest, TemperatureConversion) {
	auto result = convert(0.0, Unit::DegC, Unit::DegF);
	ASSERT_TRUE(result.has_value());
	EXPECT_NEAR(*result, 32.0, kEpsilon);
}

TEST(ConvertTest, KelvinConversion) {
	auto result = convert(273.15, Unit::Kelvin, Unit::DegC);
	ASSERT_TRUE(result.has_value());
	EXPECT_NEAR(*result, 0.0, kEpsilon);
}

TEST(ConvertTest, SpeedConversion) {
	auto result = convert(100.0, Unit::Km_h, Unit::Mi_h);
	ASSERT_TRUE(result.has_value());
	EXPECT_NEAR(*result, 62.14, kEpsilon);
}

TEST(ConvertTest, PressureConversion) {
	auto result = convert(101325.0, Unit::Pascal, Unit::Millibar);
	ASSERT_TRUE(result.has_value());
	EXPECT_NEAR(*result, 1013.25, kEpsilon);
}

TEST(ConvertTest, UnsupportedConversion) {
	auto result = convert(42.0, Unit::DegC, Unit::Meter);
	EXPECT_FALSE(result.has_value());
}

TEST(ConvertTest, LengthConversion) {
	auto result = convert(25.4, Unit::Millimeter, Unit::Inch);
	ASSERT_TRUE(result.has_value());
	EXPECT_NEAR(*result, 1.0, kEpsilon);
}

// ===== Measurement =====

TEST(MeasurementTest, HasValue) {
	Measurement m{42.0, Unit::DegC};
	EXPECT_TRUE(m.has_value());
}

TEST(MeasurementTest, NoValue) {
	Measurement m{std::nullopt, Unit::DegC};
	EXPECT_FALSE(m.has_value());
}

TEST(MeasurementTest, ConvertTo) {
	Measurement m{0.0, Unit::DegC};
	auto f = m.to(Unit::DegF);
	ASSERT_TRUE(f.has_value());
	EXPECT_NEAR(*f.value, 32.0, kEpsilon);
	EXPECT_EQ(f.unit, Unit::DegF);
}

TEST(MeasurementTest, ConvertNullValue) {
	Measurement m{std::nullopt, Unit::DegC};
	auto f = m.to(Unit::DegF);
	EXPECT_FALSE(f.has_value());
	EXPECT_EQ(f.unit, Unit::DegF);
}

// ===== QuantitativeValue =====

TEST(QuantitativeValueTest, AsMeasurement) {
	QuantitativeValue qv;
	qv.value = 20.0;
	qv.unit = Unit::DegC;
	auto m = qv.as_measurement();
	EXPECT_TRUE(m.has_value());
	EXPECT_EQ(m.unit, Unit::DegC);
	EXPECT_NEAR(*m.value, 20.0, kEpsilon);
}

TEST(QuantitativeValueTest, ConvertedTo) {
	QuantitativeValue qv;
	qv.value = 0.0;
	qv.min_value = -10.0;
	qv.max_value = 10.0;
	qv.unit = Unit::DegC;
	auto converted = qv.converted_to(Unit::DegF);
	ASSERT_TRUE(converted.value.has_value());
	EXPECT_NEAR(*converted.value, 32.0, kEpsilon);
	ASSERT_TRUE(converted.min_value.has_value());
	EXPECT_NEAR(*converted.min_value, 14.0, kEpsilon);
	ASSERT_TRUE(converted.max_value.has_value());
	EXPECT_NEAR(*converted.max_value, 50.0, kEpsilon);
	EXPECT_EQ(converted.unit, Unit::DegF);
}

TEST(QuantitativeValueTest, HasValueFalseWhenNull) {
	QuantitativeValue qv;
	EXPECT_FALSE(qv.has_value());
}

} // namespace
} // namespace nws
