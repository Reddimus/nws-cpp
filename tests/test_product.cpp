// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/cli_report.hpp"
#include "nws/models/nbp_bulletin.hpp"

#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace nws {
namespace {

constexpr const char* kCli = "000\n"
							 "CDUS41 KOKX 161000\n"
							 "CLINYC\n"
							 "\n"
							 "CLIMATE REPORT\n"
							 "NATIONAL WEATHER SERVICE NEW YORK NY\n"
							 "600 AM EDT FRI MAY 16 2026\n"
							 "\n"
							 "...THE NEW YORK CENTRAL PARK CLIMATE SUMMARY FOR MAY 16 2026...\n"
							 "\n"
							 "TEMPERATURE (F)\n"
							 " YESTERDAY\n"
							 "  MAXIMUM         72   320 PM\n"
							 "  MINIMUM         55   500 AM\n"
							 "PRECIPITATION (IN)\n"
							 "  YESTERDAY        0.00\n";

TEST(CliReport, ParsesProductText) {
	Result<CliReport> r = parse_cli_product_text(kCli);
	ASSERT_TRUE(r.has_value()) << r.error().message;
	EXPECT_EQ(r->date, "2026-05-16");
	ASSERT_TRUE(r->tmax_f.has_value());
	ASSERT_TRUE(r->tmin_f.has_value());
	EXPECT_DOUBLE_EQ(*r->tmax_f, 72.0);
	EXPECT_DOUBLE_EQ(*r->tmin_f, 55.0);
	EXPECT_FALSE(r->is_corrected);
	EXPECT_TRUE(r->is_preliminary);
	EXPECT_FALSE(r->station.empty());
	EXPECT_EQ(r->source, "nws_api_cli");
}

TEST(CliReport, DetectsCorrectedReissue) {
	std::string corrected = std::string("CDUS41 KOKX 161200 CCA\nCLINYC...CORRECTED\n") + kCli;
	Result<CliReport> r = parse_cli_product_text(corrected);
	ASSERT_TRUE(r.has_value());
	EXPECT_TRUE(r->is_corrected);
}

TEST(CliReport, RejectsEmpty) {
	EXPECT_FALSE(parse_cli_product_text("").has_value());
}

// CF6 sample modelled on the live `api.weather.gov` `CF6ATL` product format
// (May 2026). Verifies the fixed-width tabular parser branch.
constexpr const char* kCf6 =
	"\n000\n"
	"CXUS52 KFFC 241010\n"
	"CF6ATL\n"
	"PRELIMINARY LOCAL CLIMATOLOGICAL DATA (WS FORM: F-6)\n"
	"\n"
	"                                          STATION:   ATLANTA\n"
	"                                          MONTH:     MAY\n"
	"                                          YEAR:      2026\n"
	"                                          LATITUDE:   33 39 N      \n"
	"                                          LONGITUDE:  84 26 W    \n"
	"\n"
	"  TEMPERATURE IN F:       :PCPN:    SNOW:  WIND      :SUNSHINE: SKY     :PK WND \n"
	"================================================================================\n"
	"1   2   3   4   5  6A  6B    7    8   9   10  11  12  13   14  15   16   17  18\n"
	"                                     12Z  AVG MX 2MIN\n"
	"DY MAX MIN AVG DEP HDD CDD  WTR  SNW DPTH SPD SPD DIR MIN PSBL S-S WX    SPD DR\n"
	"================================================================================\n"
	"\n"
	" 1  66  57  62  -5   3   0    T  0.0    0  5.6 12  50   M    M  10        15  70\n"
	" 2  72  53  63  -5   2   0 0.01  0.0    0  9.3 26 310   M    M   6        37 320\n"
	"22  88  70  79   6   0  14 0.05  0.0    M  8.9 22 220   M    M   9 3      30 220\n"
	"23  85  69  77   4   0  12 0.43  0.0    0  8.5 17  80   M    M   9 13     22 100\n"
	"================================================================================\n"
	"SM 1848 1400        12 147  1.01  0.0    172.1          M      134              \n"
	"================================================================================\n";

TEST(CliReport, ParsesCf6Product) {
	Result<CliReport> r = parse_cli_product_text(kCf6);
	ASSERT_TRUE(r.has_value()) << r.error().message;
	// CF6 sample's most-recent populated day is May 23, 2026 — high 85, low 69.
	EXPECT_EQ(r->date, "2026-05-23");
	ASSERT_TRUE(r->tmax_f.has_value());
	ASSERT_TRUE(r->tmin_f.has_value());
	EXPECT_DOUBLE_EQ(*r->tmax_f, 85.0);
	EXPECT_DOUBLE_EQ(*r->tmin_f, 69.0);
	EXPECT_EQ(r->source, "nws_api_cf6");
	EXPECT_TRUE(r->is_preliminary); // sample carries the PRELIMINARY banner
	EXPECT_FALSE(r->is_corrected);
	EXPECT_EQ(r->station, "KATL"); // 3-letter ATL + K prefix
}

TEST(CliReport, ParsesIemJson) {
	const std::string json = R"({"results":[)"
							 R"({"station":"KNYC","valid":"2026-05-16","high":72,"low":55},)"
							 R"({"station":"KNYC","valid":"2026-05-15","high":70,"low":52}]})";
	Result<std::vector<CliReport>> r = parse_iem_cli_json(json);
	ASSERT_TRUE(r.has_value()) << r.error().message;
	ASSERT_EQ(r->size(), 2u);
	EXPECT_EQ((*r)[0].date, "2026-05-16");
	ASSERT_TRUE((*r)[0].tmax_f.has_value());
	EXPECT_DOUBLE_EQ(*(*r)[0].tmax_f, 72.0);
	EXPECT_DOUBLE_EQ(*(*r)[0].tmin_f, 55.0);
	EXPECT_EQ((*r)[0].source, "iem_json");
}

constexpr const char* kNbp = " KNYC   NBM V4.2 NBP GUIDANCE   5/17/2026  1200 UTC\n"
							 " UTC  18 00 06 12\n"
							 " TXN  68 55 52 70\n"
							 " TXNP5 67 54 51 69\n"
							 " TXNSD  3  2  2  3\n"
							 "$$\n"
							 " KBOS   NBM V4.2 NBP GUIDANCE   5/17/2026  1200 UTC\n"
							 " UTC  18 00 06 12\n"
							 " TXN  60 48 45 62\n"
							 "$$\n";

TEST(NbpBulletin, ParsesStationsRowsColumns) {
	Result<std::vector<NbpStation>> r = parse_nbp_bulletin(kNbp);
	ASSERT_TRUE(r.has_value()) << r.error().message;
	ASSERT_EQ(r->size(), 2u);

	const NbpStation& nyc = (*r)[0];
	EXPECT_EQ(nyc.station, "KNYC");
	EXPECT_NE(nyc.cycle_utc.find("1200 UTC"), std::string::npos);
	ASSERT_EQ(nyc.columns.size(), 4u);
	EXPECT_EQ(nyc.columns[0], "18");
	EXPECT_EQ(nyc.columns[3], "12");

	bool saw_p5 = false;
	for (const NbpRow& row : nyc.rows) {
		if (row.label == "TXNP5") {
			saw_p5 = true;
			ASSERT_EQ(row.values.size(), 4u);
			ASSERT_TRUE(row.values[0].has_value());
			EXPECT_EQ(*row.values[0], 67);
			EXPECT_EQ(*row.values[3], 69);
		}
	}
	EXPECT_TRUE(saw_p5);

	EXPECT_EQ((*r)[1].station, "KBOS");
}

TEST(NbpBulletin, RejectsEmpty) {
	EXPECT_FALSE(parse_nbp_bulletin("").has_value());
}

} // namespace
} // namespace nws
