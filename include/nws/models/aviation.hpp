#pragma once

#include "nws/error.hpp"
#include "nws/geo.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nws {

/// Properties for a SIGMET (Significant Meteorological Information)
struct SigmetProperties {
	std::string id;
	std::string atsu;
	std::string sequence;
	std::string phenomenon;
	std::string valid_time_from;
	std::string valid_time_to;
};

using SigmetFeature = GeoJsonFeature<SigmetProperties>;
using SigmetCollectionResponse = GeoJsonFeatureCollection<SigmetProperties>;

/// Properties for a Center Weather Advisory
struct CWAProperties {
	std::string id;
	std::string cwsu;
	std::string sequence;
	std::string text;
	std::string valid_time_from;
	std::string valid_time_to;
};

using CWAFeature = GeoJsonFeature<CWAProperties>;
using CWACollectionResponse = GeoJsonFeatureCollection<CWAProperties>;

[[nodiscard]] Result<void> deserialize_sigmet_feature(std::string_view body, SigmetFeature& out);
[[nodiscard]] Result<void> deserialize_sigmet_collection(std::string_view body,
														 SigmetCollectionResponse& out);
[[nodiscard]] Result<void> deserialize_cwa_feature(std::string_view body, CWAFeature& out);
[[nodiscard]] Result<void> deserialize_cwa_collection(std::string_view body,
													  CWACollectionResponse& out);

} // namespace nws
