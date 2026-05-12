// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
//
// Microbenchmark: parse a representative NWS gridpoint-forecast response
// 1k times and report wall-clock. Used as a parse-throughput regression
// guard with `ctest --timeout` and an absolute upper bound on us/op.
//
// Why gridpoint-forecast as the bench target: this is the hot path for
// kalshi-trader and polymarket-trader, which fetch gridpoint forecasts
// every poll cycle. 60+ optional meteorological layers, each a
// {uom, values:[~50 time-values]} block. The /forecast (12-hour periods)
// endpoint is a smaller cousin.
//
// Historical baseline (recorded at migration time on x86_64-v3, GCC 13.3,
// -O3 -DNDEBUG, payload=~43 KB, iters=1000):
//
//     nlohmann/json v3.11.3 : ~700 us/op  (pre-migration baseline)
//     glaze v7.6.0          : ~205 us/op  (post-migration steady-state)
//     speedup               :   ~3.4x
//
// Numbers measured with `ctest` thread contention in flight; quiet runs
// drop both sides ~25 percent (nlohmann ~520 us/op, glaze ~155 us/op).
//
// The pre-migration nlohmann path was removed along with the FetchContent
// dep. Re-introduce a side-by-side bench only if a future regression
// suspicion warrants it.

#include "nws/models/gridpoint.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>

namespace {

// 14 of the 60+ possible NWS gridpoint layers, each with ~50 time-value
// pairs. ~43 KB of JSON, which mirrors the wire-size of a real gridpoint
// response when filtered to the layers a typical trader uses.
std::string make_payload() {
	std::string json;
	json.reserve(64 * 1024);
	json += R"({"@context":[],"type":"Feature",)";
	json +=
		R"("geometry":{"type":"Polygon","coordinates":[[[-97.11,39.72],[-97.11,39.74],[-97.13,39.74],[-97.13,39.72],[-97.11,39.72]]]},)";
	json += R"("properties":{)";
	json += R"("updateTime":"2026-04-12T22:46:28+00:00",)";
	json += R"("validTimes":"2026-04-12T16:00:00+00:00/P7DT12H",)";
	json += R"("gridId":"TOP","gridX":32,"gridY":81,)";
	json += R"("elevation":{"unitCode":"wmoUnit:m","value":456.8952})";

	constexpr int kPoints = 50;
	const char* layers[] = {"temperature",
							"dewpoint",
							"maxTemperature",
							"minTemperature",
							"relativeHumidity",
							"apparentTemperature",
							"skyCover",
							"windDirection",
							"windSpeed",
							"windGust",
							"probabilityOfPrecipitation",
							"quantitativePrecipitation",
							"iceAccumulation",
							"snowfallAmount"};
	const char* uoms[] = {
		"wmoUnit:degC",	   "wmoUnit:degC",	 "wmoUnit:degC",	"wmoUnit:degC",
		"wmoUnit:percent", "wmoUnit:degC",	 "wmoUnit:percent", "wmoUnit:degree_(angle)",
		"wmoUnit:km_h-1",  "wmoUnit:km_h-1", "wmoUnit:percent", "wmoUnit:mm",
		"wmoUnit:mm",	   "wmoUnit:mm"};

	const std::size_t n_layers = sizeof(layers) / sizeof(layers[0]);
	for (std::size_t li = 0; li < n_layers; ++li) {
		json += ",\"";
		json += layers[li];
		json += "\":{\"uom\":\"";
		json += uoms[li];
		json += "\",\"values\":[";
		for (int i = 0; i < kPoints; ++i) {
			char buf[160];
			std::snprintf(buf, sizeof(buf),
						  "%s{\"validTime\":\"2026-04-12T%02d:00:00+00:00/PT1H\",\"value\":%.2f}",
						  i == 0 ? "" : ",", i % 24,
						  10.0 + 5.0 * std::sin(i * 0.13 + static_cast<double>(li)));
			json += buf;
		}
		json += "]}";
	}
	json += "}}";
	return json;
}

} // namespace

int main() {
	const std::string payload = make_payload();
	constexpr int kIterations = 1000;

	// Warmup — let the allocator and CPU settle.
	for (int i = 0; i < 50; ++i) {
		nws::GridpointResponse warm;
		(void)nws::deserialize_gridpoint_response(payload, warm);
	}

	std::chrono::nanoseconds glaze_total{0};
	std::size_t glaze_checksum = 0;
	for (int i = 0; i < kIterations; ++i) {
		nws::GridpointResponse resp;
		std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
		nws::Result<void> r = nws::deserialize_gridpoint_response(payload, resp);
		std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
		if (!r) {
			std::fprintf(stderr, "glaze parse failed: %s\n", r.error().message.c_str());
			return 1;
		}
		glaze_total += (t1 - t0);
		if (resp.properties.temperature) {
			glaze_checksum += resp.properties.temperature->values.size();
		}
	}

	// 14 layers x 50 points and we only checksum temperature (50/iter)
	if (glaze_checksum != static_cast<std::size_t>(50) * kIterations) {
		std::fprintf(stderr, "checksum mismatch: glaze=%zu (expected %d)\n", glaze_checksum,
					 50 * kIterations);
		return 1;
	}

	const double glaze_ms = glaze_total.count() / 1e6;
	const double us_per_op = (glaze_total.count() / 1e3) / kIterations;

	std::printf("parse_benchmark: payload=%zuB iters=%d\n", payload.size(), kIterations);
	std::printf("  glaze: %8.3f ms total  (%8.3f us/op)\n", glaze_ms, us_per_op);

	// Regression guard: at migration time, Glaze parsed this payload at
	// ~205 us/op steady-state on x86_64-v3 with ctest thread contention
	// in flight (~155 us/op standalone). Set the cap at 600 us/op — that's
	// still meaningfully faster than the nlohmann baseline (~700 us/op
	// under the same contention) and leaves a healthy slack window for
	// slower CI runners, Debug builds, and noisy macOS runners.
	constexpr double kMaxUsPerOp = 600.0;
	if (us_per_op > kMaxUsPerOp) {
		std::fprintf(stderr, "REGRESSION: %.3f us/op exceeds cap of %.0f us/op\n", us_per_op,
					 kMaxUsPerOp);
		return 1;
	}
	return 0;
}
