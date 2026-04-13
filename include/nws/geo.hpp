#pragma once

#include <string>
#include <variant>
#include <vector>

namespace nws {

/// GeoJSON Point geometry
struct GeoPoint {
	double longitude{0.0};
	double latitude{0.0};
};

/// GeoJSON Polygon geometry
struct GeoPolygon {
	std::vector<std::vector<GeoPoint>> coordinates;
};

/// GeoJSON geometry (Point, Polygon, or null)
using Geometry = std::variant<GeoPoint, GeoPolygon, std::nullptr_t>;

/// GeoJSON Feature wrapper
template <typename Properties>
struct GeoJsonFeature {
	std::string id;
	std::string type{"Feature"};
	Geometry geometry{nullptr};
	Properties properties;
};

/// GeoJSON FeatureCollection wrapper
template <typename Properties>
struct GeoJsonFeatureCollection {
	std::string type{"FeatureCollection"};
	std::vector<GeoJsonFeature<Properties>> features;
};

} // namespace nws
