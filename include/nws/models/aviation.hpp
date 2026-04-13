#pragma once

#include "nws/geo.hpp"

#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
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

void from_json(const nlohmann::json& j, SigmetProperties& p);
void from_json(const nlohmann::json& j, SigmetFeature& r);
void from_json(const nlohmann::json& j, SigmetCollectionResponse& r);
void from_json(const nlohmann::json& j, CWAProperties& p);
void from_json(const nlohmann::json& j, CWAFeature& r);
void from_json(const nlohmann::json& j, CWACollectionResponse& r);

} // namespace nws
