// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
#pragma once

/// @file cli_report.hpp
/// @brief NWS Daily Climate Report (CLI) / F6 (CF6) parsing.
///
/// Kalshi/Polymarket/ForecastEx daily-temperature contracts settle on the
/// official CLI/CF6 max/min — NOT on raw ASOS or the slower NCEI-final
/// value. Two ingestion paths produce a `CliReport`:
///
///  - `parse_cli_product_text()` — the authoritative source-of-record:
///    the raw text of an `api.weather.gov` CLI/CF6 product
///    (`ProductProperties::product_text`).
///  - `parse_iem_cli_json()` — the fast bulk path: Iowa Environmental
///    Mesonet's already-parsed `json/cli.py` feed (a year of rows).

#include "nws/error.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nws {

/// One station-day climate settlement record.
struct CliReport {
	std::string station;		  ///< ICAO/CLI site id (e.g. "KNYC")
	std::string date;			  ///< ISO local calendar day "YYYY-MM-DD"
	std::optional<double> tmax_f; ///< observed daily maximum (deg F)
	std::optional<double> tmin_f; ///< observed daily minimum (deg F)
	bool is_preliminary{true};	  ///< CLI morning issuance is preliminary
	bool is_corrected{false};	  ///< product was reissued ("...COR")
	std::string source;			  ///< "nws_api_cli" | "nws_api_cf6" | "iem_json"
	std::string issuance_time;	  ///< raw issuance timestamp if known
};

/// Parse the raw text of an api.weather.gov CLI (or CF6) product.
[[nodiscard]] Result<CliReport> parse_cli_product_text(std::string_view product_text);

/// Parse Iowa Environmental Mesonet's `json/cli.py` payload — returns every
/// station-day row it contains (typically a full year for one station).
[[nodiscard]] Result<std::vector<CliReport>> parse_iem_cli_json(std::string_view json);

} // namespace nws
