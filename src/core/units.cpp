#include "nws/units.hpp"

#include <cmath>
#include <unordered_map>

namespace nws {

Unit parse_unit_code(std::string_view unit_code) noexcept {
	// Strip common prefixes
	static const std::unordered_map<std::string_view, Unit> unit_map = {
		// Temperature
		{"wmoUnit:degC", Unit::DegC},
		{"wmoUnit:degF", Unit::DegF},
		{"wmoUnit:K", Unit::Kelvin},

		// Speed
		{"wmoUnit:km_h-1", Unit::Km_h},
		{"wmoUnit:m_s-1", Unit::M_s},
		{"wmoUnit:kt", Unit::Knot},
		{"wmoUnit:mi_h-1", Unit::Mi_h},

		// Distance
		{"wmoUnit:m", Unit::Meter},
		{"wmoUnit:km", Unit::Kilometer},
		{"unit:mi", Unit::Mile},

		// Pressure
		{"wmoUnit:Pa", Unit::Pascal},
		{"wmoUnit:mbar", Unit::Millibar},
		{"wmoUnit:hPa", Unit::Millibar}, // hectopascal = millibar

		// Precipitation
		{"wmoUnit:mm", Unit::Millimeter},
		{"unit:in", Unit::Inch},
		{"wmoUnit:cm", Unit::Centimeter},

		// Angular
		{"wmoUnit:degree_(angle)", Unit::DegreeAngle},

		// Other
		{"wmoUnit:percent", Unit::Percent},
	};

	auto it = unit_map.find(unit_code);
	if (it != unit_map.end()) {
		return it->second;
	}
	return Unit::Unknown;
}

// Temperature conversions
double celsius_to_fahrenheit(double c) noexcept {
	return c * 9.0 / 5.0 + 32.0;
}
double fahrenheit_to_celsius(double f) noexcept {
	return (f - 32.0) * 5.0 / 9.0;
}

// Speed conversions
double kmh_to_mph(double kmh) noexcept {
	return kmh * 0.621371;
}
double mph_to_kmh(double mph) noexcept {
	return mph / 0.621371;
}
double ms_to_mph(double ms) noexcept {
	return ms * 2.23694;
}
double mph_to_ms(double mph) noexcept {
	return mph / 2.23694;
}
double ms_to_kmh(double ms) noexcept {
	return ms * 3.6;
}
double knots_to_mph(double kt) noexcept {
	return kt * 1.15078;
}
double mph_to_knots(double mph) noexcept {
	return mph / 1.15078;
}

// Pressure conversions
double pascal_to_millibar(double pa) noexcept {
	return pa / 100.0;
}
double millibar_to_pascal(double mb) noexcept {
	return mb * 100.0;
}

// Length conversions
double mm_to_inches(double mm) noexcept {
	return mm / 25.4;
}
double inches_to_mm(double in) noexcept {
	return in * 25.4;
}
double meters_to_miles(double m) noexcept {
	return m / 1609.344;
}
double km_to_miles(double km) noexcept {
	return km * 0.621371;
}

std::optional<double> convert(double value, Unit from, Unit to) noexcept {
	if (from == to) {
		return value;
	}

	// Temperature
	if (from == Unit::DegC && to == Unit::DegF) {
		return celsius_to_fahrenheit(value);
	}
	if (from == Unit::DegF && to == Unit::DegC) {
		return fahrenheit_to_celsius(value);
	}
	if (from == Unit::Kelvin && to == Unit::DegC) {
		return value - 273.15;
	}
	if (from == Unit::DegC && to == Unit::Kelvin) {
		return value + 273.15;
	}
	if (from == Unit::Kelvin && to == Unit::DegF) {
		return celsius_to_fahrenheit(value - 273.15);
	}
	if (from == Unit::DegF && to == Unit::Kelvin) {
		return fahrenheit_to_celsius(value) + 273.15;
	}

	// Speed
	if (from == Unit::Km_h && to == Unit::Mi_h) {
		return kmh_to_mph(value);
	}
	if (from == Unit::Mi_h && to == Unit::Km_h) {
		return mph_to_kmh(value);
	}
	if (from == Unit::M_s && to == Unit::Mi_h) {
		return ms_to_mph(value);
	}
	if (from == Unit::Mi_h && to == Unit::M_s) {
		return mph_to_ms(value);
	}
	if (from == Unit::M_s && to == Unit::Km_h) {
		return ms_to_kmh(value);
	}
	if (from == Unit::Km_h && to == Unit::M_s) {
		return value / 3.6;
	}
	if (from == Unit::Knot && to == Unit::Mi_h) {
		return knots_to_mph(value);
	}
	if (from == Unit::Mi_h && to == Unit::Knot) {
		return mph_to_knots(value);
	}
	if (from == Unit::Knot && to == Unit::Km_h) {
		return value * 1.852;
	}
	if (from == Unit::Km_h && to == Unit::Knot) {
		return value / 1.852;
	}
	if (from == Unit::Knot && to == Unit::M_s) {
		return value * 0.514444;
	}
	if (from == Unit::M_s && to == Unit::Knot) {
		return value / 0.514444;
	}

	// Pressure
	if (from == Unit::Pascal && to == Unit::Millibar) {
		return pascal_to_millibar(value);
	}
	if (from == Unit::Millibar && to == Unit::Pascal) {
		return millibar_to_pascal(value);
	}

	// Length
	if (from == Unit::Millimeter && to == Unit::Inch) {
		return mm_to_inches(value);
	}
	if (from == Unit::Inch && to == Unit::Millimeter) {
		return inches_to_mm(value);
	}
	if (from == Unit::Meter && to == Unit::Mile) {
		return meters_to_miles(value);
	}
	if (from == Unit::Kilometer && to == Unit::Mile) {
		return km_to_miles(value);
	}
	if (from == Unit::Mile && to == Unit::Kilometer) {
		return value / 0.621371;
	}
	if (from == Unit::Meter && to == Unit::Kilometer) {
		return value / 1000.0;
	}
	if (from == Unit::Kilometer && to == Unit::Meter) {
		return value * 1000.0;
	}
	if (from == Unit::Centimeter && to == Unit::Inch) {
		return value / 2.54;
	}
	if (from == Unit::Inch && to == Unit::Centimeter) {
		return value * 2.54;
	}
	if (from == Unit::Centimeter && to == Unit::Millimeter) {
		return value * 10.0;
	}
	if (from == Unit::Millimeter && to == Unit::Centimeter) {
		return value / 10.0;
	}

	return std::nullopt;
}

Measurement Measurement::to(Unit target) const {
	if (!value.has_value()) {
		return {std::nullopt, target};
	}
	std::optional<double> result = convert(*value, unit, target);
	if (result.has_value()) {
		return {*result, target};
	}
	return {std::nullopt, target};
}

QuantitativeValue QuantitativeValue::converted_to(Unit target) const {
	QuantitativeValue result = *this;
	if (value.has_value()) {
		std::optional<double> converted = convert(*value, unit, target);
		if (converted.has_value()) {
			result.value = *converted;
			result.unit = target;
		}
	}
	if (max_value.has_value()) {
		std::optional<double> converted = convert(*max_value, unit, target);
		if (converted.has_value()) {
			result.max_value = *converted;
		}
	}
	if (min_value.has_value()) {
		std::optional<double> converted = convert(*min_value, unit, target);
		if (converted.has_value()) {
			result.min_value = *converted;
		}
	}
	return result;
}

} // namespace nws
