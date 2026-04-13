#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

namespace nws {

/// A single glossary term and its definition
struct GlossaryTerm {
	std::string term;
	std::string definition;
};

/// Response from the /glossary endpoint
struct GlossaryResponse {
	std::vector<GlossaryTerm> glossary;
};

void from_json(const nlohmann::json& j, GlossaryTerm& t);
void from_json(const nlohmann::json& j, GlossaryResponse& r);

} // namespace nws
