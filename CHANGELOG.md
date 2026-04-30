# Changelog

All notable changes to **nws-cpp** are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project
uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### CI

- Add `.markdownlint-cli2.yaml` mirroring the open-meteo-cpp config —
  disables `MD013` (line-length) and other style-noise rules but keeps
  `MD022`/`MD031`/`MD032` formatting hygiene enforced.
- Fix `CLAUDE.md` formatting (blank lines around headings and fenced
  code blocks) so the markdown-lint job passes.

### Build

- Enforce explicit local cpp types ([`b90f909`](https://github.com/Reddimus/nws-cpp/commit/b90f909)).

### Refactor

- Replace `std::ostringstream` with `std::format` throughout
  ([`6e85e8f`](https://github.com/Reddimus/nws-cpp/commit/6e85e8f)).

## [0.1.0] — 2026-04-15

### Added

- Initial release: C++23 NWS API SDK
- Layered static libraries: `nws_core` → `nws_http` → `nws_models` →
  `nws_api` → `nws` (INTERFACE)
- `std::expected<T, Error>` for all returns; no exceptions
- Pimpl pattern for `HttpClient` and `NWSClient`
- Endpoints: points, gridpoints, forecasts (regular + hourly),
  observations, stations, alerts (7 variants), zones, glossary
- Convenience wrappers: `get_forecast_for_location()`,
  `get_current_weather()`
- Mermaid architecture diagram + API references in README
- 137 unit tests via GoogleTest (FetchContent)

[Unreleased]: https://github.com/Reddimus/nws-cpp/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/Reddimus/nws-cpp/releases/tag/v0.1.0
