// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/glossary.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string_view>
#include <utility>

#include "glaze_detail.hpp"

namespace nws {

Result<void> deserialize_glossary_response(std::string_view body, GlossaryResponse& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}

	const glz::generic* glossary = detail::find_array(*root, "glossary");
	if (glossary == nullptr) {
		return {};
	}
	const glz::generic::array_t& arr = glossary->get_array();
	out.glossary.clear();
	out.glossary.reserve(arr.size());
	for (const glz::generic& term_json : arr) {
		GlossaryTerm term;
		term.term = detail::get_string(term_json, "term");
		term.definition = detail::get_string(term_json, "definition");
		out.glossary.push_back(std::move(term));
	}
	return {};
}

} // namespace nws
