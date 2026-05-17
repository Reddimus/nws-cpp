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

Result<CliReport> parse_cli_product_text(std::string_view product_text) {
	if (product_text.empty()) {
		return std::unexpected(Error::parse("empty CLI product text"));
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
