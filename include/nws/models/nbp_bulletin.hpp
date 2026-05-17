// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
#pragma once

/// @file nbp_bulletin.hpp
/// @brief NOAA National Blend of Models — NBP (probabilistic) text bulletin.
///
/// NBP is a per-station fixed-width text product (NOMADS `…/blend/prod/`)
/// carrying calibrated MaxT/MinT percentiles + standard deviation:
/// `TXNP1`…`TXNP9` (the 10/25/50/75/90th percentiles), `TXNSD`
/// (standard deviation), `TXN` (the QMD mean Max/Min). No GRIB needed.
///
/// The parser is intentionally low-level: it returns each labelled row's
/// values aligned to the forecast-column header, and leaves the
/// label→percentile interpretation to the consumer (nws-data), so the SDK
/// stays robust to NBM table revisions.

#include "nws/error.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nws {

/// One labelled row (e.g. "TXNP5") with one value per forecast column.
struct NbpRow {
	std::string label;
	std::vector<std::optional<int>> values; // aligned to NbpStation::columns
};

/// One station block within an NBP bulletin.
struct NbpStation {
	std::string station;			  ///< e.g. "KNYC"
	std::string cycle_utc;			  ///< raw cycle stamp from the header
	std::vector<std::string> columns; ///< forecast-column labels (UTC/FHR row)
	std::vector<NbpRow> rows;		  ///< labelled data rows in file order
};

/// Parse a full NBP bulletin (may contain many station blocks).
[[nodiscard]] Result<std::vector<NbpStation>> parse_nbp_bulletin(std::string_view text);

} // namespace nws
