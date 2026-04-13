#pragma once

#include "nws/units.hpp"

#include <nlohmann/json_fwd.hpp>
#include <string>

namespace nws {

/// Parse a QuantitativeValue from NWS JSON
void from_json(const nlohmann::json& j, QuantitativeValue& qv);

/// Parse a GeoPoint from GeoJSON coordinates
void from_json(const nlohmann::json& j, struct GeoPoint& p);

/// Safely extract a string from JSON (handles null values)
[[nodiscard]] std::string json_string(const nlohmann::json& j, const char* key);

/// Safely extract an int from JSON (handles null values)
[[nodiscard]] std::int32_t json_int(const nlohmann::json& j, const char* key, std::int32_t def = 0);

} // namespace nws
