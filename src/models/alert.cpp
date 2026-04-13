#include "nws/models/alert.hpp"

#include "nws/models/common.hpp"

#include <nlohmann/json.hpp>

namespace nws {

AlertSeverity parse_alert_severity(std::string_view s) {
	if (s == "Extreme")
		return AlertSeverity::Extreme;
	if (s == "Severe")
		return AlertSeverity::Severe;
	if (s == "Moderate")
		return AlertSeverity::Moderate;
	if (s == "Minor")
		return AlertSeverity::Minor;
	return AlertSeverity::Unknown;
}

AlertCertainty parse_alert_certainty(std::string_view s) {
	if (s == "Observed")
		return AlertCertainty::Observed;
	if (s == "Likely")
		return AlertCertainty::Likely;
	if (s == "Possible")
		return AlertCertainty::Possible;
	if (s == "Unlikely")
		return AlertCertainty::Unlikely;
	return AlertCertainty::Unknown;
}

AlertUrgency parse_alert_urgency(std::string_view s) {
	if (s == "Immediate")
		return AlertUrgency::Immediate;
	if (s == "Expected")
		return AlertUrgency::Expected;
	if (s == "Future")
		return AlertUrgency::Future;
	if (s == "Past")
		return AlertUrgency::Past;
	return AlertUrgency::Unknown;
}

AlertStatus parse_alert_status(std::string_view s) {
	if (s == "Actual")
		return AlertStatus::Actual;
	if (s == "Exercise")
		return AlertStatus::Exercise;
	if (s == "System")
		return AlertStatus::System;
	if (s == "Test")
		return AlertStatus::Test;
	if (s == "Draft")
		return AlertStatus::Draft;
	return AlertStatus::Actual;
}

AlertMessageType parse_alert_message_type(std::string_view s) {
	if (s == "Alert")
		return AlertMessageType::Alert;
	if (s == "Update")
		return AlertMessageType::Update;
	if (s == "Cancel")
		return AlertMessageType::Cancel;
	if (s == "Ack")
		return AlertMessageType::Ack;
	if (s == "Error")
		return AlertMessageType::Error;
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

void from_json(const nlohmann::json& j, AlertProperties& p) {
	p.id = json_string(j, "id");
	p.area_desc = json_string(j, "areaDesc");

	if (j.contains("geocode") && !j["geocode"].is_null()) {
		const auto& gc = j["geocode"];
		if (gc.contains("UGC") && gc["UGC"].is_array()) {
			p.geocode.ugc = gc["UGC"].get<std::vector<std::string>>();
		}
		if (gc.contains("SAME") && gc["SAME"].is_array()) {
			p.geocode.same = gc["SAME"].get<std::vector<std::string>>();
		}
	}

	if (j.contains("affectedZones") && j["affectedZones"].is_array()) {
		p.affected_zones = j["affectedZones"].get<std::vector<std::string>>();
	}

	p.sent = json_string(j, "sent");
	p.effective = json_string(j, "effective");
	if (j.contains("onset") && j["onset"].is_string()) {
		p.onset = j["onset"].get<std::string>();
	}
	p.expires = json_string(j, "expires");
	if (j.contains("ends") && j["ends"].is_string()) {
		p.ends = j["ends"].get<std::string>();
	}

	p.status = parse_alert_status(json_string(j, "status"));
	p.message_type = parse_alert_message_type(json_string(j, "messageType"));
	p.category = json_string(j, "category");
	p.severity = parse_alert_severity(json_string(j, "severity"));
	p.certainty = parse_alert_certainty(json_string(j, "certainty"));
	p.urgency = parse_alert_urgency(json_string(j, "urgency"));
	p.event = json_string(j, "event");
	p.sender = json_string(j, "sender");
	p.sender_name = json_string(j, "senderName");

	if (j.contains("headline") && j["headline"].is_string()) {
		p.headline = j["headline"].get<std::string>();
	}
	p.description = json_string(j, "description");
	if (j.contains("instruction") && j["instruction"].is_string()) {
		p.instruction = j["instruction"].get<std::string>();
	}
	p.response = json_string(j, "response");
}

void from_json(const nlohmann::json& j, AlertFeature& r) {
	r.id = json_string(j, "id");
	r.type = j.contains("type") && j["type"].is_string() ? j["type"].get<std::string>() : "Feature";

	if (j.contains("geometry") && !j["geometry"].is_null()) {
		GeoPoint gp;
		from_json(j["geometry"], gp);
		r.geometry = gp;
	}

	if (j.contains("properties") && !j["properties"].is_null()) {
		from_json(j["properties"], r.properties);
	}
}

void from_json(const nlohmann::json& j, AlertCollectionResponse& r) {
	r.type = j.contains("type") && j["type"].is_string() ? j["type"].get<std::string>()
														 : "FeatureCollection";
	if (j.contains("features") && j["features"].is_array()) {
		for (const auto& feat : j["features"]) {
			AlertFeature alert;
			from_json(feat, alert);
			r.features.push_back(std::move(alert));
		}
	}
}

void from_json(const nlohmann::json& j, AlertActiveCount& c) {
	c.total = json_int(j, "total");
	c.land = json_int(j, "land");
	c.marine = json_int(j, "marine");
	if (j.contains("regions") && j["regions"].is_object()) {
		for (auto& [key, val] : j["regions"].items()) {
			c.regions[key] = val.get<std::int32_t>();
		}
	}
	if (j.contains("areas") && j["areas"].is_object()) {
		for (auto& [key, val] : j["areas"].items()) {
			c.areas[key] = val.get<std::int32_t>();
		}
	}
	if (j.contains("zones") && j["zones"].is_object()) {
		for (auto& [key, val] : j["zones"].items()) {
			c.zones[key] = val.get<std::int32_t>();
		}
	}
}

} // namespace nws
