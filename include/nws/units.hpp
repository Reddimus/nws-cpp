#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace nws {

/// WMO/UCUM unit codes returned by the NWS API
enum class Unit : std::uint8_t {
	// Temperature
	DegC,	// wmoUnit:degC
	DegF,	// wmoUnit:degF
	Kelvin, // wmoUnit:K

	// Speed
	Km_h, // wmoUnit:km_h-1
	M_s,  // wmoUnit:m_s-1
	Knot, // wmoUnit:kt
	Mi_h, // wmoUnit:mi_h-1 (mph)

	// Distance/Length
	Meter,	   // wmoUnit:m
	Kilometer, // wmoUnit:km
	Mile,	   // unit:mi

	// Pressure
	Pascal,	  // wmoUnit:Pa
	Millibar, // wmoUnit:mbar / hPa

	// Precipitation
	Millimeter, // wmoUnit:mm
	Inch,		// unit:in
	Centimeter, // wmoUnit:cm

	// Angular
	DegreeAngle, // wmoUnit:degree_(angle)

	// Other
	Percent, // wmoUnit:percent

	Unknown
};

/// Convert Unit enum to display string
[[nodiscard]] constexpr std::string_view to_string(Unit unit) noexcept {
	switch (unit) {
		case Unit::DegC:
			return "C";
		case Unit::DegF:
			return "F";
		case Unit::Kelvin:
			return "K";
		case Unit::Km_h:
			return "km/h";
		case Unit::M_s:
			return "m/s";
		case Unit::Knot:
			return "kt";
		case Unit::Mi_h:
			return "mph";
		case Unit::Meter:
			return "m";
		case Unit::Kilometer:
			return "km";
		case Unit::Mile:
			return "mi";
		case Unit::Pascal:
			return "Pa";
		case Unit::Millibar:
			return "mbar";
		case Unit::Millimeter:
			return "mm";
		case Unit::Inch:
			return "in";
		case Unit::Centimeter:
			return "cm";
		case Unit::DegreeAngle:
			return "deg";
		case Unit::Percent:
			return "%";
		case Unit::Unknown:
			return "?";
	}
	return "?";
}

/// Parse a WMO/UCUM unit code string into a Unit enum
[[nodiscard]] Unit parse_unit_code(std::string_view unit_code) noexcept;

/// Parsed measurement with value and unit
struct Measurement {
	std::optional<double> value;
	Unit unit{Unit::Unknown};

	[[nodiscard]] bool has_value() const noexcept { return value.has_value(); }

	/// Convert to a different unit
	[[nodiscard]] Measurement to(Unit target) const;
};

/// QuantitativeValue from NWS API responses
struct QuantitativeValue {
	std::optional<double> value;
	std::string unit_code; // raw string e.g. "wmoUnit:degC"
	Unit unit{Unit::Unknown};
	std::optional<double> max_value;
	std::optional<double> min_value;
	std::optional<std::string> quality_control;

	[[nodiscard]] bool has_value() const noexcept { return value.has_value(); }

	[[nodiscard]] Measurement as_measurement() const { return {value, unit}; }

	/// Get value converted to target unit
	[[nodiscard]] QuantitativeValue converted_to(Unit target) const;
};

// Temperature conversions
[[nodiscard]] double celsius_to_fahrenheit(double c) noexcept;
[[nodiscard]] double fahrenheit_to_celsius(double f) noexcept;

// Speed conversions
[[nodiscard]] double kmh_to_mph(double kmh) noexcept;
[[nodiscard]] double mph_to_kmh(double mph) noexcept;
[[nodiscard]] double ms_to_mph(double ms) noexcept;
[[nodiscard]] double mph_to_ms(double mph) noexcept;
[[nodiscard]] double ms_to_kmh(double ms) noexcept;
[[nodiscard]] double knots_to_mph(double kt) noexcept;
[[nodiscard]] double mph_to_knots(double mph) noexcept;

// Pressure conversions
[[nodiscard]] double pascal_to_millibar(double pa) noexcept;
[[nodiscard]] double millibar_to_pascal(double mb) noexcept;

// Length conversions
[[nodiscard]] double mm_to_inches(double mm) noexcept;
[[nodiscard]] double inches_to_mm(double in) noexcept;
[[nodiscard]] double meters_to_miles(double m) noexcept;
[[nodiscard]] double km_to_miles(double km) noexcept;

/// Convert a value from one unit to another. Returns nullopt if conversion is unsupported.
[[nodiscard]] std::optional<double> convert(double value, Unit from, Unit to) noexcept;

} // namespace nws
