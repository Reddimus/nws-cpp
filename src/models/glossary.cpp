#include "nws/models/glossary.hpp"

#include <nlohmann/json.hpp>

namespace nws {

void from_json(const nlohmann::json& j, GlossaryTerm& t) {
	t.term = j.value("term", "");
	t.definition = j.value("definition", "");
}

void from_json(const nlohmann::json& j, GlossaryResponse& r) {
	if (j.contains("glossary") && j["glossary"].is_array()) {
		r.glossary.clear();
		for (const auto& term_json : j["glossary"]) {
			GlossaryTerm term;
			from_json(term_json, term);
			r.glossary.push_back(std::move(term));
		}
	}
}

} // namespace nws
