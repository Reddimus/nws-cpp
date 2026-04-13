# nws-cpp

C++23 SDK for the [National Weather Service API](https://api.weather.gov/) (api.weather.gov).

Provides typed, `std::expected`-based access to NWS weather data including forecasts, observations, alerts, and more. No API key required.

## Quick Start

```cpp
#include "nws/nws.hpp"
#include <iostream>

int main() {
    nws::NWSClient::Config config;
    config.http.user_agent = "(myapp, contact@example.com)";
    nws::NWSClient client(std::move(config));

    // Get forecast for Austin, TX
    auto forecast = client.get_forecast_for_location(30.2672, -97.7431);
    if (!forecast) {
        std::cerr << forecast.error().message << "\n";
        return 1;
    }

    for (const auto& period : forecast->properties.periods) {
        std::cout << period.name << ": " << period.temperature
                  << period.temperature_unit << " - " << period.short_forecast << "\n";
    }
}
```

## Features

- **C++23** with `std::expected<T, Error>` for all return types (no exceptions)
- **All core NWS endpoints**: Points, Forecasts, Observations, Stations, Alerts
- **Unit conversion**: WMO unit codes parsed into typed enums with conversion functions
- **Rate limiting**: Built-in token bucket rate limiter (NWS-tuned defaults)
- **Retry with backoff**: Configurable exponential backoff with jitter
- **Points cache**: Optional LRU cache for lat/lon -> grid coordinate lookups
- **GeoJSON parsing**: Typed `GeoJsonFeature<T>` and `GeoJsonFeatureCollection<T>` wrappers
- **RFC 7807 errors**: NWS Problem Details responses parsed into structured `Error` objects
- **Pimpl pattern**: ABI-stable; implementation details hidden from public headers

## Building

### Requirements

- C++23 compiler (GCC 13+, Clang 16+)
- CMake 3.20+
- libcurl

### Build

```bash
make build        # Release build
make test         # Run tests
make lint         # Check formatting
make format       # Format code
make coverage     # Generate coverage report (requires lcov)
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `NWS_BUILD_TESTS` | `ON` | Build unit tests |
| `NWS_BUILD_EXAMPLES` | `ON` | Build examples |
| `NWS_ENABLE_LTO` | `ON` | Link-time optimization |
| `NWS_NATIVE_ARCH` | `OFF` | `-march=native` tuning |
| `NWS_ENABLE_SANITIZERS` | `OFF` | ASan + UBSan |
| `NWS_ENABLE_COVERAGE` | `OFF` | gcov instrumentation |

### Using as a Dependency

```cmake
find_package(nws CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE nws::nws)
```

Or with FetchContent:

```cmake
FetchContent_Declare(nws-cpp
    GIT_REPOSITORY https://github.com/Reddimus/nws-cpp.git
    GIT_TAG main
)
FetchContent_MakeAvailable(nws-cpp)
target_link_libraries(myapp PRIVATE nws)
```

## Architecture

```
nws_core   ── error, rate_limit, retry, units
nws_http   ── libcurl HTTP client (Pimpl)
nws_models ── GeoJSON models + from_json parsers
nws_api    ── NWSClient (all endpoints)
nws        ── INTERFACE aggregator
```

## API Coverage

### Endpoints

| Category | Endpoints |
|----------|-----------|
| Points | `get_point()` |
| Gridpoints | `get_gridpoint()`, `get_forecast()`, `get_forecast_hourly()`, `get_gridpoint_stations()` |
| Observations | `get_observations()`, `get_latest_observation()` |
| Stations | `get_station()` |
| Alerts | `get_alerts()`, `get_active_alerts()`, `get_active_alert_count()`, `get_active_alerts_by_area()`, `get_active_alerts_by_zone()`, `get_alert()`, `get_alert_types()` |
| Zones | `get_zone()`, `get_zone_forecast()` |
| Glossary | `get_glossary()` |

### Convenience Methods

| Method | Description |
|--------|-------------|
| `get_forecast_for_location(lat, lon)` | Auto-resolves point -> grid -> forecast |
| `get_current_weather(lat, lon)` | Auto-resolves point -> station -> latest observation |

### Model Types (all with typed from_json parsers)

Point, Forecast, Observation, Station, Alert, Zone, Gridpoint, Office, Product, Aviation (SIGMET/CWA), Radar, Glossary

### Model Types

| Type | Fields |
|------|--------|
| `PointProperties` | gridId, gridX, gridY, forecast URLs, timeZone, radarStation |
| `ForecastPeriod` | temperature, wind, precipitation probability, text forecast |
| `ObservationProperties` | temperature, dewpoint, wind, pressure, humidity, clouds |
| `StationProperties` | identifier, name, timeZone, elevation |
| `AlertProperties` | severity, urgency, certainty, event, headline, description |
| `QuantitativeValue` | value, unitCode, qualityControl + unit conversion |

## Examples

```bash
make run-basic_usage       # Point lookup
make run-get_forecast      # 14-period forecast
make run-get_observations  # Current weather with unit conversion
make run-get_alerts        # Active alerts by state
make run-weather_monitor   # Polling station every 60s
```

## NWS API Notes

- **No API key** required. A `User-Agent` header is mandatory.
- **Rate limiting**: Undisclosed thresholds; SDK defaults to 5 req/sec with retry on 503.
- **Response format**: GeoJSON (`application/geo+json`) with nested `properties` objects.
- **Errors**: RFC 7807 Problem Details with `correlationId` for debugging.

## Dependencies

| Library | Purpose | Integration |
|---------|---------|-------------|
| libcurl | HTTP requests | `find_package(CURL)` |
| nlohmann/json | JSON parsing | `FetchContent` |
| GoogleTest | Unit testing | `FetchContent` |

## License

MIT
