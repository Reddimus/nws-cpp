#pragma once

#include "nws/error.hpp"

#include <string>
#include <string_view>
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

[[nodiscard]] Result<void> deserialize_glossary_response(std::string_view body,
														 GlossaryResponse& out);

} // namespace nws
