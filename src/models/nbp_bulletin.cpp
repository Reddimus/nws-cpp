// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/nbp_bulletin.hpp"

#include <cctype>
#include <charconv>
#include <string>
#include <string_view>

namespace nws {

namespace {

std::vector<std::string> tokens(std::string_view s) {
	std::vector<std::string> out;
	std::size_t i = 0;
	while (i < s.size()) {
		while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
			++i;
		}
		std::size_t j = i;
		while (j < s.size() && !std::isspace(static_cast<unsigned char>(s[j]))) {
			++j;
		}
		if (j > i) {
			out.emplace_back(s.substr(i, j - i));
		}
		i = j;
	}
	return out;
}

std::optional<int> as_value(const std::string& t) {
	// NBM missing/sentinel markers.
	if (t.empty() || t == "999" || t == "-99" || t == "NG" || t == "**") {
		return std::nullopt;
	}
	int v = 0;
	const std::from_chars_result r = std::from_chars(t.data(), t.data() + t.size(), v);
	if (r.ec != std::errc{} || r.ptr != t.data() + t.size()) {
		return std::nullopt;
	}
	return v;
}

bool contains(std::string_view hay, std::string_view needle) {
	return hay.find(needle) != std::string_view::npos;
}

} // namespace

Result<std::vector<NbpStation>> parse_nbp_bulletin(std::string_view text) {
	if (text.empty()) {
		return std::unexpected(Error::parse("empty NBP bulletin"));
	}

	std::vector<NbpStation> stations;
	NbpStation cur;
	bool in_block = false;
	bool have_columns = false;

	std::size_t start = 0;
	while (start <= text.size()) {
		std::size_t nl = text.find('\n', start);
		std::string_view line =
			(nl == std::string_view::npos) ? text.substr(start) : text.substr(start, nl - start);
		if (!line.empty() && line.back() == '\r') {
			line.remove_suffix(1);
		}
		const std::size_t next = (nl == std::string_view::npos) ? text.size() + 1 : nl + 1;
		start = next;

		// Station header: "<STN>   NBM ... NBP GUIDANCE  M/D/YYYY  HHMM UTC"
		if (contains(line, " NBP ") && contains(line, "GUIDANCE")) {
			if (in_block && !cur.station.empty()) {
				stations.push_back(std::move(cur));
			}
			cur = NbpStation{};
			in_block = true;
			have_columns = false;
			const std::vector<std::string> tk = tokens(line);
			if (!tk.empty()) {
				cur.station = tk.front();
			}
			std::size_t gp = line.find("GUIDANCE");
			if (gp != std::string_view::npos) {
				std::string_view tail = line.substr(gp + 8);
				std::size_t a = tail.find_first_not_of(" \t");
				if (a != std::string_view::npos) {
					cur.cycle_utc = std::string(tail.substr(a));
				}
			}
			continue;
		}
		if (!in_block) {
			continue;
		}
		if (contains(line, "$$")) { // end of a station block
			if (!cur.station.empty()) {
				stations.push_back(std::move(cur));
			}
			cur = NbpStation{};
			in_block = false;
			have_columns = false;
			continue;
		}

		const std::vector<std::string> tk = tokens(line);
		if (tk.empty()) {
			continue;
		}
		const std::string& label = tk.front();

		// First UTC/FHR row defines the forecast columns.
		if (!have_columns && (label == "UTC" || label == "FHR" || label == "DT")) {
			for (std::size_t i = 1; i < tk.size(); ++i) {
				cur.columns.push_back(tk[i]);
			}
			have_columns = true;
			continue;
		}

		// Any other labelled row → values aligned to the columns.
		NbpRow row;
		row.label = label;
		for (std::size_t i = 1; i < tk.size(); ++i) {
			row.values.push_back(as_value(tk[i]));
		}
		cur.rows.push_back(std::move(row));
	}

	if (in_block && !cur.station.empty()) {
		stations.push_back(std::move(cur));
	}
	if (stations.empty()) {
		return std::unexpected(Error::parse("no NBP station blocks found"));
	}
	return stations;
}

} // namespace nws
