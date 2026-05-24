// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/cli_report.hpp"

#include <array>
#include <cctype>
#include <charconv>
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <string_view>

#include "glaze_detail.hpp"

namespace nws {

namespace {

std::vector<std::string_view> lines_of(std::string_view text) {
	std::vector<std::string_view> out;
	std::size_t start = 0;
	while (start <= text.size()) {
		std::size_t nl = text.find('\n', start);
		if (nl == std::string_view::npos) {
			out.push_back(text.substr(start));
			break;
		}
		std::string_view ln = text.substr(start, nl - start);
		if (!ln.empty() && ln.back() == '\r') {
			ln.remove_suffix(1);
		}
		out.push_back(ln);
		start = nl + 1;
	}
	return out;
}

bool icontains(std::string_view hay, std::string_view needle) {
	if (needle.size() > hay.size()) {
		return false;
	}
	for (std::size_t i = 0; i + needle.size() <= hay.size(); ++i) {
		bool ok = true;
		for (std::size_t j = 0; j < needle.size(); ++j) {
			if (std::toupper(static_cast<unsigned char>(hay[i + j])) !=
				std::toupper(static_cast<unsigned char>(needle[j]))) {
				ok = false;
				break;
			}
		}
		if (ok) {
			return true;
		}
	}
	return false;
}

// First signed integer token in `s`, if any.
std::optional<double> first_int(std::string_view s) {
	for (std::size_t i = 0; i < s.size(); ++i) {
		const unsigned char c = static_cast<unsigned char>(s[i]);
		if (std::isdigit(c) ||
			(c == '-' && i + 1 < s.size() && std::isdigit(static_cast<unsigned char>(s[i + 1])))) {
			std::size_t j = i + 1;
			while (j < s.size() && std::isdigit(static_cast<unsigned char>(s[j]))) {
				++j;
			}
			int v = 0;
			const std::from_chars_result r = std::from_chars(s.data() + i, s.data() + j, v);
			if (r.ec == std::errc{}) {
				return static_cast<double>(v);
			}
		}
	}
	return std::nullopt;
}

// Month number from the first WHOLE word in `line` whose first three
// letters are a month abbreviation. Word-based (not substring) so e.g.
// "SUMMARY" does not falsely match "MAR".
int month_num(std::string_view line) {
	static constexpr std::array<const char*, 12> kM = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
													   "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
	std::size_t i = 0;
	while (i < line.size()) {
		while (i < line.size() && !std::isalpha(static_cast<unsigned char>(line[i]))) {
			++i;
		}
		std::size_t j = i;
		while (j < line.size() && std::isalpha(static_cast<unsigned char>(line[j]))) {
			++j;
		}
		if (j - i >= 3) {
			std::array<char, 3> w{};
			for (std::size_t k = 0; k < 3; ++k) {
				w[k] = static_cast<char>(std::toupper(static_cast<unsigned char>(line[i + k])));
			}
			for (int m = 0; m < 12; ++m) {
				const char* abbr = kM[static_cast<std::size_t>(m)];
				if (w[0] == abbr[0] && w[1] == abbr[1] && w[2] == abbr[2]) {
					return m + 1;
				}
			}
		}
		i = j;
	}
	return 0;
}

} // namespace

namespace {

/// Parse the fixed-width tabular CF6 (Preliminary Local Climatological Data,
/// WS Form F-6) layout. CF6 differs from CLI by carrying a full month of
/// daily rows; we return the most recent day with both tmax and tmin present.
/// CF6 sample row: " 1  66  57  62  -5   3   0    T  0.0 ..."
/// columns: DY MAX MIN AVG DEP HDD CDD WTR SNW ...
Result<CliReport> parse_cf6_product_text(std::string_view product_text) {
	CliReport r;
	r.source = "nws_api_cf6";
	r.is_preliminary = icontains(product_text, "PRELIMINARY");
	r.is_corrected = icontains(product_text, "CORRECTED") || icontains(product_text, "...COR");

	int month = 0;
	int year = 0;
	std::string station_3letter;
	bool past_data_header = false;
	int best_day = 0;
	std::optional<double> best_tmax;
	std::optional<double> best_tmin;

	const std::vector<std::string_view> ls = lines_of(product_text);
	for (std::string_view ln : ls) {
		// "CF6ATL" -> station_3letter = "ATL"
		if (station_3letter.empty()) {
			std::size_t cf6_pos = ln.find("CF6");
			if (cf6_pos != std::string_view::npos && cf6_pos + 6 <= ln.size()) {
				std::string_view cand = ln.substr(cf6_pos + 3, 3);
				bool valid = cand.size() == 3;
				for (char c : cand) {
					if (!std::isupper(static_cast<unsigned char>(c))) {
						valid = false;
						break;
					}
				}
				if (valid) {
					station_3letter = std::string(cand);
				}
			}
		}

		if (month == 0 && icontains(ln, "MONTH:")) {
			const int m = month_num(ln);
			if (m != 0) {
				month = m;
			}
		}

		if (year == 0 && icontains(ln, "YEAR:")) {
			std::size_t p = 0;
			while (p < ln.size()) {
				while (p < ln.size() && !std::isdigit(static_cast<unsigned char>(ln[p]))) {
					++p;
				}
				std::size_t q = p;
				while (q < ln.size() && std::isdigit(static_cast<unsigned char>(ln[q]))) {
					++q;
				}
				if (q > p) {
					int v = 0;
					std::from_chars(ln.data() + p, ln.data() + q, v);
					if (v >= 1900 && v <= 2200) {
						year = v;
						break;
					}
				}
				p = q + 1;
			}
		}

		// Data header line carries both "DY" near start and "MAX" + "MIN" elsewhere.
		if (!past_data_header) {
			const std::size_t dy = ln.find("DY");
			if (dy != std::string_view::npos && dy < 5 &&
				ln.find("MAX") != std::string_view::npos &&
				ln.find("MIN") != std::string_view::npos) {
				past_data_header = true;
				continue;
			}
		} else {
			// Trim leading whitespace + skip blanks / separators.
			std::size_t s = 0;
			while (s < ln.size() && std::isspace(static_cast<unsigned char>(ln[s]))) {
				++s;
			}
			if (s >= ln.size() || ln[s] == '=') {
				continue;
			}
			// Stop on summary rows ("SM ...", "AV ...", "NOTES:", "$$").
			std::string_view body = ln.substr(s);
			if (body.starts_with("SM ") || body.starts_with("AV ") || body.starts_with("NOTES") ||
				body.starts_with("$$") || body.starts_with("MAX(") || body.starts_with("MISC")) {
				break;
			}
			// Tokenize whitespace-separated columns.
			std::vector<std::string_view> toks;
			std::size_t i = s;
			while (i < ln.size()) {
				while (i < ln.size() && std::isspace(static_cast<unsigned char>(ln[i]))) {
					++i;
				}
				std::size_t j = i;
				while (j < ln.size() && !std::isspace(static_cast<unsigned char>(ln[j]))) {
					++j;
				}
				if (j > i) {
					toks.push_back(ln.substr(i, j - i));
				}
				i = j;
			}
			if (toks.size() < 3) {
				continue;
			}
			// Token 0 must be a day (1..31).
			int day = 0;
			if (std::from_chars(toks[0].data(), toks[0].data() + toks[0].size(), day).ec !=
					std::errc{} ||
				day < 1 || day > 31) {
				continue;
			}
			// Tokens 1 and 2 = MAX, MIN. "M" means missing.
			auto parse_temp = [](std::string_view t) -> std::optional<double> {
				if (t == "M" || t == "T") {
					return std::nullopt;
				}
				int v = 0;
				if (std::from_chars(t.data(), t.data() + t.size(), v).ec != std::errc{}) {
					return std::nullopt;
				}
				return static_cast<double>(v);
			};
			const std::optional<double> tmax = parse_temp(toks[1]);
			const std::optional<double> tmin = parse_temp(toks[2]);
			if (tmax.has_value() && tmin.has_value() && day > best_day) {
				best_day = day;
				best_tmax = tmax;
				best_tmin = tmin;
			}
		}
	}

	if (month == 0 || year == 0 || best_day == 0) {
		return std::unexpected(Error::parse("unrecognized CLI/CF6 product layout"));
	}
	char buf[11];
	std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, best_day);
	r.date = buf;
	if (!station_3letter.empty()) {
		r.station = "K" + station_3letter; // CONUS K-prefix
	}
	r.tmax_f = best_tmax;
	r.tmin_f = best_tmin;
	return r;
}

} // namespace

Result<CliReport> parse_cli_product_text(std::string_view product_text) {
	if (product_text.empty()) {
		return std::unexpected(Error::parse("empty CLI product text"));
	}
	// CF6 (PRELIMINARY LOCAL CLIMATOLOGICAL DATA, WS Form F-6) uses a fixed-
	// width tabular format that the CLI keyword parser below cannot read.
	// Detect on the F-6 banner and route to the CF6-specific parser.
	if (icontains(product_text, "WS FORM: F-6") || icontains(product_text, "WS FORM F-6") ||
		icontains(product_text, "WS FORM:  F-6")) {
		return parse_cf6_product_text(product_text);
	}
	CliReport r;
	r.source = "nws_api_cli";
	r.is_preliminary = true;
	r.is_corrected = icontains(product_text, "CORRECTED") || icontains(product_text, "...COR");

	const std::vector<std::string_view> ls = lines_of(product_text);
	bool in_temperature = false;
	for (std::string_view raw : ls) {
		std::string_view ln = raw;

		// WMO/AWIPS header: "CDUS41 KOKX 170800" -> capture the K-ICAO.
		if (r.station.empty()) {
			std::size_t kpos = ln.find(" K");
			if (kpos != std::string_view::npos && kpos + 5 <= ln.size()) {
				std::string_view cand = ln.substr(kpos + 1, 4);
				bool icao = cand.size() == 4 && cand[0] == 'K';
				for (std::size_t j = 1; j < cand.size() && icao; ++j) {
					if (!std::isupper(static_cast<unsigned char>(cand[j]))) {
						icao = false;
					}
				}
				if (icao) {
					r.station = std::string(cand);
				}
			}
		}

		// "CLIMATE SUMMARY FOR MAY 16 2026" / "...FOR MAY 16 2026"
		if (r.date.empty() && icontains(ln, "FOR ") && month_num(ln) != 0) {
			const int mm = month_num(ln);
			std::optional<double> dd;
			std::optional<double> yyyy;
			// scan tokens after the month for day then year
			std::size_t p = 0;
			while (p < ln.size()) {
				while (p < ln.size() && !std::isdigit(static_cast<unsigned char>(ln[p]))) {
					++p;
				}
				std::size_t q = p;
				while (q < ln.size() && std::isdigit(static_cast<unsigned char>(ln[q]))) {
					++q;
				}
				if (q > p) {
					int v = 0;
					std::from_chars(ln.data() + p, ln.data() + q, v);
					if (!dd && v >= 1 && v <= 31) {
						dd = static_cast<double>(v);
					} else if (!yyyy && v >= 1900 && v <= 2200) {
						yyyy = static_cast<double>(v);
					}
				}
				p = q + 1;
			}
			if (dd && yyyy) {
				char buf[11];
				std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", static_cast<int>(*yyyy), mm,
							  static_cast<int>(*dd));
				r.date = buf;
			}
		}

		if (icontains(ln, "TEMPERATURE")) {
			in_temperature = true;
			continue;
		}
		if (in_temperature && (icontains(ln, "PRECIPITATION") || icontains(ln, "WIND") ||
							   icontains(ln, "RELATIVE"))) {
			in_temperature = false;
		}
		if (in_temperature && !r.tmax_f && icontains(ln, "MAXIMUM")) {
			r.tmax_f = first_int(ln.substr(ln.find("MAXIMUM") + 7));
		}
		if (in_temperature && !r.tmin_f && icontains(ln, "MINIMUM")) {
			r.tmin_f = first_int(ln.substr(ln.find("MINIMUM") + 7));
		}
	}

	if (r.station.empty() && r.date.empty() && !r.tmax_f && !r.tmin_f) {
		return std::unexpected(Error::parse("unrecognized CLI/CF6 product layout"));
	}
	return r;
}

Result<std::vector<CliReport>> parse_iem_cli_json(std::string_view json) {
	Result<glz::generic> root = detail::parse_root(json);
	if (!root) {
		return std::unexpected(root.error());
	}
	const glz::generic* results = detail::find_array(*root, "results");
	if (!results) {
		results = detail::find_array(*root, "data");
	}
	if (!results) {
		return std::unexpected(Error::parse("IEM cli.py: no results array"));
	}
	std::vector<CliReport> out;
	const glz::generic::array_t& arr = results->get_array();
	for (const glz::generic& row : arr) {
		CliReport r;
		r.source = "iem_json";
		r.is_preliminary = true;
		r.station = detail::get_string(row, "station");
		r.date = detail::get_string(row, "valid");
		if (r.date.empty()) {
			r.date = detail::get_string(row, "date");
		}
		r.tmax_f = detail::get_optional_double(row, "high");
		r.tmin_f = detail::get_optional_double(row, "low");
		if (!r.date.empty()) {
			out.push_back(std::move(r));
		}
	}
	return out;
}

} // namespace nws
