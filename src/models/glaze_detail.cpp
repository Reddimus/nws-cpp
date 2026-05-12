// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "glaze_detail.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <string_view>
#include <variant>

namespace nws::detail {

namespace {

const glz::generic::object_t* as_object(const glz::generic& node) {
	if (!node.is_object()) {
		return nullptr;
	}
	return &node.get_object();
}

const glz::generic* lookup(const glz::generic& obj, const char* key) {
	const glz::generic::object_t* o = as_object(obj);
	if (o == nullptr) {
		return nullptr;
	}
	glz::generic::object_t::const_iterator it = o->find(key);
	if (it == o->end()) {
		return nullptr;
	}
	return &it->second;
}

} // namespace

std::string get_string(const glz::generic& obj, const char* key) {
	const glz::generic* v = lookup(obj, key);
	if (v == nullptr || !v->is_string()) {
		return {};
	}
	return v->get<std::string>();
}

std::int32_t get_int(const glz::generic& obj, const char* key, std::int32_t def) {
	const glz::generic* v = lookup(obj, key);
	if (v == nullptr || !v->is_number()) {
		return def;
	}
	return static_cast<std::int32_t>(v->get<double>());
}

std::optional<double> get_optional_double(const glz::generic& obj, const char* key) {
	const glz::generic* v = lookup(obj, key);
	if (v == nullptr || !v->is_number()) {
		return std::nullopt;
	}
	return v->get<double>();
}

std::optional<std::string> get_optional_string(const glz::generic& obj, const char* key) {
	const glz::generic* v = lookup(obj, key);
	if (v == nullptr || !v->is_string()) {
		return std::nullopt;
	}
	return v->get<std::string>();
}

bool get_bool(const glz::generic& obj, const char* key, bool def) {
	const glz::generic* v = lookup(obj, key);
	if (v == nullptr || !v->is_boolean()) {
		return def;
	}
	return v->get<bool>();
}

std::vector<std::string> get_string_array(const glz::generic& obj, const char* key) {
	const glz::generic* v = lookup(obj, key);
	if (v == nullptr || !v->is_array()) {
		return {};
	}
	const glz::generic::array_t& arr = v->get_array();
	std::vector<std::string> out;
	out.reserve(arr.size());
	for (const glz::generic& elem : arr) {
		if (elem.is_string()) {
			out.push_back(elem.get<std::string>());
		}
	}
	return out;
}

const glz::generic* find_object(const glz::generic& obj, const char* key) {
	const glz::generic* v = lookup(obj, key);
	if (v == nullptr || !v->is_object()) {
		return nullptr;
	}
	return v;
}

const glz::generic* find_array(const glz::generic& obj, const char* key) {
	const glz::generic* v = lookup(obj, key);
	if (v == nullptr || !v->is_array()) {
		return nullptr;
	}
	return v;
}

void parse_quantitative_value(const glz::generic& node, QuantitativeValue& qv) {
	if (node.is_null() || !node.is_object()) {
		return;
	}
	qv.unit_code = get_string(node, "unitCode");
	qv.unit = parse_unit_code(qv.unit_code);
	qv.value = get_optional_double(node, "value");
	qv.max_value = get_optional_double(node, "maxValue");
	qv.min_value = get_optional_double(node, "minValue");
	qv.quality_control = get_optional_string(node, "qualityControl");
}

std::optional<GeoPoint> parse_geopoint(const glz::generic& node) {
	// Object form: {"type":"Point","coordinates":[lon,lat]}
	if (node.is_object()) {
		const glz::generic* coords = find_array(node, "coordinates");
		if (coords == nullptr) {
			return std::nullopt;
		}
		const glz::generic::array_t& arr = coords->get_array();
		if (arr.size() < 2 || !arr[0].is_number() || !arr[1].is_number()) {
			return std::nullopt;
		}
		GeoPoint p;
		p.longitude = arr[0].get<double>();
		p.latitude = arr[1].get<double>();
		return p;
	}
	// Bare-array form: [lon,lat]
	if (node.is_array()) {
		const glz::generic::array_t& arr = node.get_array();
		if (arr.size() < 2 || !arr[0].is_number() || !arr[1].is_number()) {
			return std::nullopt;
		}
		GeoPoint p;
		p.longitude = arr[0].get<double>();
		p.latitude = arr[1].get<double>();
		return p;
	}
	return std::nullopt;
}

Geometry parse_geometry(const glz::generic& node) {
	if (node.is_null() || !node.is_object()) {
		return nullptr;
	}
	const std::string type = get_string(node, "type");
	if (type == "Point") {
		std::optional<GeoPoint> gp = parse_geopoint(node);
		if (gp) {
			return *gp;
		}
		return nullptr;
	}
	if (type == "Polygon") {
		GeoPolygon poly;
		const glz::generic* outer = find_array(node, "coordinates");
		if (outer == nullptr) {
			return poly;
		}
		const glz::generic::array_t& outer_arr = outer->get_array();
		poly.coordinates.reserve(outer_arr.size());
		for (const glz::generic& ring : outer_arr) {
			if (!ring.is_array()) {
				continue;
			}
			std::vector<GeoPoint> ring_pts;
			const glz::generic::array_t& ring_arr = ring.get_array();
			ring_pts.reserve(ring_arr.size());
			for (const glz::generic& pt : ring_arr) {
				if (pt.is_array()) {
					const glz::generic::array_t& pa = pt.get_array();
					if (pa.size() >= 2 && pa[0].is_number() && pa[1].is_number()) {
						GeoPoint gp;
						gp.longitude = pa[0].get<double>();
						gp.latitude = pa[1].get<double>();
						ring_pts.push_back(gp);
					}
				}
			}
			poly.coordinates.push_back(std::move(ring_pts));
		}
		return poly;
	}
	return nullptr;
}

Result<glz::generic> parse_root(std::string_view body) {
	glz::generic root{};
	glz::error_ctx ec = glz::read_json(root, body);
	if (ec) {
		return std::unexpected(Error::parse(glz::format_error(ec, body)));
	}
	return root;
}

} // namespace nws::detail
