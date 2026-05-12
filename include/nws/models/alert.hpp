#pragma once

#include "nws/error.hpp"
#include "nws/geo.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nws {

enum class AlertSeverity : std::uint8_t { Extreme, Severe, Moderate, Minor, Unknown };
enum class AlertCertainty : std::uint8_t { Observed, Likely, Possible, Unlikely, Unknown };
enum class AlertUrgency : std::uint8_t { Immediate, Expected, Future, Past, Unknown };
enum class AlertStatus : std::uint8_t { Actual, Exercise, System, Test, Draft };
enum class AlertMessageType : std::uint8_t { Alert, Update, Cancel, Ack, Error };

[[nodiscard]] AlertSeverity parse_alert_severity(std::string_view s);
[[nodiscard]] AlertCertainty parse_alert_certainty(std::string_view s);
[[nodiscard]] AlertUrgency parse_alert_urgency(std::string_view s);
[[nodiscard]] AlertStatus parse_alert_status(std::string_view s);
[[nodiscard]] AlertMessageType parse_alert_message_type(std::string_view s);

[[nodiscard]] std::string_view to_string(AlertSeverity v);
[[nodiscard]] std::string_view to_string(AlertCertainty v);
[[nodiscard]] std::string_view to_string(AlertUrgency v);

struct AlertGeocodes {
	std::vector<std::string> ugc;
	std::vector<std::string> same;
};

struct AlertProperties {
	std::string id;
	std::string area_desc;
	AlertGeocodes geocode;
	std::vector<std::string> affected_zones;
	std::string sent;
	std::string effective;
	std::optional<std::string> onset;
	std::string expires;
	std::optional<std::string> ends;
	AlertStatus status{AlertStatus::Actual};
	AlertMessageType message_type{AlertMessageType::Alert};
	std::string category;
	AlertSeverity severity{AlertSeverity::Unknown};
	AlertCertainty certainty{AlertCertainty::Unknown};
	AlertUrgency urgency{AlertUrgency::Unknown};
	std::string event;
	std::string sender;
	std::string sender_name;
	std::optional<std::string> headline;
	std::string description;
	std::optional<std::string> instruction;
	std::string response;
};

using AlertFeature = GeoJsonFeature<AlertProperties>;
using AlertCollectionResponse = GeoJsonFeatureCollection<AlertProperties>;

struct AlertActiveCount {
	std::int32_t total{0};
	std::int32_t land{0};
	std::int32_t marine{0};
	std::map<std::string, std::int32_t> regions;
	std::map<std::string, std::int32_t> areas;
	std::map<std::string, std::int32_t> zones;
};

[[nodiscard]] Result<void> deserialize_alert_feature(std::string_view body, AlertFeature& out);
[[nodiscard]] Result<void> deserialize_alert_collection(std::string_view body,
														AlertCollectionResponse& out);
[[nodiscard]] Result<void> deserialize_alert_active_count(std::string_view body,
														  AlertActiveCount& out);

} // namespace nws
