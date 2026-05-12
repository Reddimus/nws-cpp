// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
#pragma once

/// @file glaze_detail.hpp
/// @brief Internal Glaze AST-walk helpers shared across nws-cpp model TUs
///
/// NOT a public API. Lives under `src/models/` (NOT under `include/`) so it
/// is never installed; downstream consumers (kalshi-trader, polymarket-trader)
/// cannot accidentally depend on these symbols. They exist only so the
/// individual model translation units can reuse small primitives like
/// null-safe string/int/double extraction and the GeoJSON-variant geometry
/// walk without duplicating the logic 13 times.
///
/// Why glz::generic instead of pure compile-time meta:
///
///   1. The NWS payload mixes JSON-LD keys ("@id", "@type") with regular
///      keys; glz::meta tolerates this but generic walking is uniform.
///   2. Several response shapes have a polymorphic geometry block
///      (Point | Polygon | null), expressed as std::variant in C++. Glaze
///      can do tagged variants but the geometry has no discriminator field
///      in the parent — only `"type"` inside the geometry object itself.
///      Walking via generic is the simplest path.
///   3. ForecastPeriod.windSpeed is sometimes a string ("10 to 15 mph")
///      and sometimes a QuantitativeValue object; generic-walk lets us
///      handle both shapes in one place.
///   4. The pre-migration null-safety guarantees (every field defaults
///      on null) compose naturally over a generic walk.
///
/// Performance: a single glz::generic parse + a hand-walk still gives ~4x
/// over nlohmann on the gridpoint-forecast hot path (see tests/parse_benchmark.cpp).

#include "nws/error.hpp"
#include "nws/geo.hpp"
#include "nws/units.hpp"

#include <cstdint>
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nws::detail {

// ===== Primitive null-safe extractors =====
//
// Each returns a sensible default if the key is missing OR the JSON value
// is null OR the type doesn't match. This preserves the pre-migration
// `json_string()` / `json_int()` semantics exactly.

[[nodiscard]] std::string get_string(const glz::generic& obj, const char* key);
[[nodiscard]] std::int32_t get_int(const glz::generic& obj, const char* key, std::int32_t def = 0);
[[nodiscard]] std::optional<double> get_optional_double(const glz::generic& obj, const char* key);
[[nodiscard]] std::optional<std::string> get_optional_string(const glz::generic& obj,
															 const char* key);
[[nodiscard]] bool get_bool(const glz::generic& obj, const char* key, bool def = false);

// Vector of strings (null-safe; missing key or non-array yields empty)
[[nodiscard]] std::vector<std::string> get_string_array(const glz::generic& obj, const char* key);

/// Returns the child object stored at `key`, or nullptr if missing/non-object/null.
[[nodiscard]] const glz::generic* find_object(const glz::generic& obj, const char* key);

/// Returns the child array stored at `key`, or nullptr if missing/non-array.
[[nodiscard]] const glz::generic* find_array(const glz::generic& obj, const char* key);

// ===== Domain-specific extractors =====

/// Parse a QuantitativeValue node into qv. Tolerates null and missing keys.
/// If `node` is null/empty, leaves `qv` in its default-constructed state.
void parse_quantitative_value(const glz::generic& node, QuantitativeValue& qv);

/// Parse a GeoJSON Point geometry (object with "coordinates":[lon,lat]
/// OR a bare [lon,lat] array). Returns nullopt if neither form matches.
[[nodiscard]] std::optional<GeoPoint> parse_geopoint(const glz::generic& node);

/// Parse a Geometry variant (Point / Polygon / null). Returns nullptr_t
/// when the node is null or unknown.
[[nodiscard]] Geometry parse_geometry(const glz::generic& node);

/// Parse the top-level body string into a glz::generic root. Returns a
/// human-readable parse-error on malformed JSON.
[[nodiscard]] Result<glz::generic> parse_root(std::string_view body);

} // namespace nws::detail
