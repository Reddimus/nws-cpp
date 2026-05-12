// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "nws/models/alert.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <string_view>
#include <utility>

#include "glaze_detail.hpp"

namespace nws {

AlertSeverity parse_alert_severity(std::string_view s) {
	if (s == "Extreme") {
		return AlertSeverity::Extreme;
	}
	if (s == "Severe") {
		return AlertSeverity::Severe;
	}
	if (s == "Moderate") {
		return AlertSeverity::Moderate;
	}
	if (s == "Minor") {
		return AlertSeverity::Minor;
	}
	return AlertSeverity::Unknown;
}

AlertCertainty parse_alert_certainty(std::string_view s) {
	if (s == "Observed") {
		return AlertCertainty::Observed;
	}
	if (s == "Likely") {
		return AlertCertainty::Likely;
	}
	if (s == "Possible") {
		return AlertCertainty::Possible;
	}
	if (s == "Unlikely") {
		return AlertCertainty::Unlikely;
	}
	return AlertCertainty::Unknown;
}

AlertUrgency parse_alert_urgency(std::string_view s) {
	if (s == "Immediate") {
		return AlertUrgency::Immediate;
	}
	if (s == "Expected") {
		return AlertUrgency::Expected;
	}
	if (s == "Future") {
		return AlertUrgency::Future;
	}
	if (s == "Past") {
		return AlertUrgency::Past;
	}
	return AlertUrgency::Unknown;
}

AlertStatus parse_alert_status(std::string_view s) {
	if (s == "Actual") {
		return AlertStatus::Actual;
	}
	if (s == "Exercise") {
		return AlertStatus::Exercise;
	}
	if (s == "System") {
		return AlertStatus::System;
	}
	if (s == "Test") {
		return AlertStatus::Test;
	}
	if (s == "Draft") {
		return AlertStatus::Draft;
	}
	return AlertStatus::Actual;
}

AlertMessageType parse_alert_message_type(std::string_view s) {
	if (s == "Alert") {
		return AlertMessageType::Alert;
	}
	if (s == "Update") {
		return AlertMessageType::Update;
	}
	if (s == "Cancel") {
		return AlertMessageType::Cancel;
	}
	if (s == "Ack") {
		return AlertMessageType::Ack;
	}
	if (s == "Error") {
		return AlertMessageType::Error;
	}
	return AlertMessageType::Alert;
}

std::string_view to_string(AlertSeverity v) {
	switch (v) {
		case AlertSeverity::Extreme:
			return "Extreme";
		case AlertSeverity::Severe:
			return "Severe";
		case AlertSeverity::Moderate:
			return "Moderate";
		case AlertSeverity::Minor:
			return "Minor";
		case AlertSeverity::Unknown:
			return "Unknown";
	}
	return "Unknown";
}

std::string_view to_string(AlertCertainty v) {
	switch (v) {
		case AlertCertainty::Observed:
			return "Observed";
		case AlertCertainty::Likely:
			return "Likely";
		case AlertCertainty::Possible:
			return "Possible";
		case AlertCertainty::Unlikely:
			return "Unlikely";
		case AlertCertainty::Unknown:
			return "Unknown";
	}
	return "Unknown";
}

std::string_view to_string(AlertUrgency v) {
	switch (v) {
		case AlertUrgency::Immediate:
			return "Immediate";
		case AlertUrgency::Expected:
			return "Expected";
		case AlertUrgency::Future:
			return "Future";
		case AlertUrgency::Past:
			return "Past";
		case AlertUrgency::Unknown:
			return "Unknown";
	}
	return "Unknown";
}

namespace {

void populate_alert_properties(const glz::generic& props, AlertProperties& p) {
	p.id = detail::get_string(props, "id");
	p.area_desc = detail::get_string(props, "areaDesc");

	const glz::generic* geocode = detail::find_object(props, "geocode");
	if (geocode != nullptr) {
		p.geocode.ugc = detail::get_string_array(*geocode, "UGC");
		p.geocode.same = detail::get_string_array(*geocode, "SAME");
	}

	p.affected_zones = detail::get_string_array(props, "affectedZones");

	p.sent = detail::get_string(props, "sent");
	p.effective = detail::get_string(props, "effective");
	p.onset = detail::get_optional_string(props, "onset");
	p.expires = detail::get_string(props, "expires");
	p.ends = detail::get_optional_string(props, "ends");

	p.status = parse_alert_status(detail::get_string(props, "status"));
	p.message_type = parse_alert_message_type(detail::get_string(props, "messageType"));
	p.category = detail::get_string(props, "category");
	p.severity = parse_alert_severity(detail::get_string(props, "severity"));
	p.certainty = parse_alert_certainty(detail::get_string(props, "certainty"));
	p.urgency = parse_alert_urgency(detail::get_string(props, "urgency"));
	p.event = detail::get_string(props, "event");
	p.sender = detail::get_string(props, "sender");
	p.sender_name = detail::get_string(props, "senderName");

	p.headline = detail::get_optional_string(props, "headline");
	p.description = detail::get_string(props, "description");
	p.instruction = detail::get_optional_string(props, "instruction");
	p.response = detail::get_string(props, "response");
}

void populate_alert_feature(const glz::generic& root, AlertFeature& r) {
	r.id = detail::get_string(root, "id");
	std::string type = detail::get_string(root, "type");
	r.type = type.empty() ? "Feature" : std::move(type);

	const glz::generic* geom = detail::find_object(root, "geometry");
	if (geom != nullptr) {
		r.geometry = detail::parse_geometry(*geom);
	}

	const glz::generic* props = detail::find_object(root, "properties");
	if (props != nullptr) {
		populate_alert_properties(*props, r.properties);
	}
}

void populate_string_int_map(const glz::generic& parent, const char* key,
							 std::map<std::string, std::int32_t>& out) {
	const glz::generic* obj = detail::find_object(parent, key);
	if (obj == nullptr) {
		return;
	}
	for (const auto& [k, v] : obj->get_object()) {
		if (v.is_number()) {
			out[k] = static_cast<std::int32_t>(v.get<double>());
		}
	}
}

} // namespace

Result<void> deserialize_alert_feature(std::string_view body, AlertFeature& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}
	populate_alert_feature(*root, out);
	return {};
}

Result<void> deserialize_alert_collection(std::string_view body, AlertCollectionResponse& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}

	std::string type = detail::get_string(*root, "type");
	out.type = type.empty() ? "FeatureCollection" : std::move(type);

	const glz::generic* features = detail::find_array(*root, "features");
	if (features == nullptr) {
		return {};
	}
	const glz::generic::array_t& arr = features->get_array();
	out.features.reserve(arr.size());
	for (const glz::generic& feat : arr) {
		AlertFeature alert;
		populate_alert_feature(feat, alert);
		out.features.push_back(std::move(alert));
	}

	return {};
}

Result<void> deserialize_alert_active_count(std::string_view body, AlertActiveCount& out) {
	Result<glz::generic> root = detail::parse_root(body);
	if (!root) {
		return std::unexpected(root.error());
	}

	out.total = detail::get_int(*root, "total");
	out.land = detail::get_int(*root, "land");
	out.marine = detail::get_int(*root, "marine");
	populate_string_int_map(*root, "regions", out.regions);
	populate_string_int_map(*root, "areas", out.areas);
	populate_string_int_map(*root, "zones", out.zones);
	return {};
}

} // namespace nws
