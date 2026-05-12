#pragma once

/// @file common.hpp
/// @brief Common model deserializers shared across all NWS API responses
///
/// Backed by [Glaze](https://github.com/stephenberry/glaze) for JSON
/// deserialization. The public surface is the struct definitions in
/// `nws/units.hpp` and `nws/geo.hpp` plus the `deserialize_*(std::string_view, T&)`
/// family used by `api/client.cpp`. The previous
/// `from_json(const nlohmann::json&, T&)` overloads and the
/// `json_string()` / `json_int()` helpers have been removed; downstream
/// consumers (kalshi-trader, polymarket-trader) only use the high-level
/// `NWSClient::get_*` methods, never these internal helpers.

#include "nws/error.hpp"
#include "nws/units.hpp"

#include <string_view>

namespace nws {

// ===== Top-level deserializer for QuantitativeValue =====
//
// Most NWS responses embed many QuantitativeValue subfields; those are
// parsed inline by the per-response Glaze walks. This helper is exposed
// only for the test suite's null-safety regression coverage.

[[nodiscard]] Result<void> deserialize_quantitative_value(std::string_view body,
														  QuantitativeValue& out);

} // namespace nws
