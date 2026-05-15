# Changelog

All notable changes to **nws-cpp** are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project
uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `.editorconfig` (fleet-standard: tabs, 4-width, LF, UTF-8, 100-col
  max for C++). Sibling to `.clang-format`; covers editors that don't
  read `.clang-format` (#12).
- `make pre-commit` + `make install-hooks` targets — auto-format + lint
  via git pre-commit hook (#7).

### Changed

- README FetchContent snippet pinned to v0.2.0 (#8).
- `cpp_auto_audit` allowlist emptied (16 entries → 0); all production
  code now uses explicit types per the `no auto` rule (#9).

### Removed

- All stale `nlohmann/from_json` claims from README (the Glaze
  migration shipped in v0.2.0 made these obsolete) (#11).

### Fixed

- LICENSE: copyright holder de-truncated (#10).

## [0.2.0] - 2026-05-12

### Build

- **JSON: nlohmann/json → Glaze v7.6.0**. Compile-time-reflection JSON
  library; ~3.4x parse speedup on the gridpoint-forecast hot path
  (~700 us/op nlohmann → ~205 us/op Glaze steady-state on a ~43 KB
  representative payload, 14 of the 60+ NWS gridpoint layers, GCC 13.3
  on x86_64-v3). The migration replaces every `from_json(const
  nlohmann::json&, T&)` overload with a `deserialize_*(std::string_view,
  T&)` family that returns `Result<void>`. Public `NWSClient::get_*`
  signatures are unchanged; only the internal model surface moved.
- Hand-walk strategy via `glz::generic`: the NWS payload mixes JSON-LD
  keys (`@id`, `@type`), polymorphic GeoJSON geometry variants (Point /
  Polygon / null), and mixed-shape fields (`ForecastPeriod.windSpeed`
  is sometimes a string and sometimes a QuantitativeValue object).
  These don't map cleanly onto a static `glz::meta`, so the migration
  parses each body once into a `glz::generic` AST and walks it via a
  small set of `detail::get_*` null-safe extractors in
  `src/models/glaze_detail.hpp` (not installed, not part of the public
  ABI).
- `tests/parse_benchmark.cpp` added — 1k-iteration regression guard on
  a synthetic gridpoint-forecast payload, wired into `ctest --timeout`.
- `tests/glaze_test.cpp` added — covers the migration's per-model parse
  behaviour (variant geometry, dynamic-key AlertActiveCount maps, mixed
  windSpeed shape, null-tolerant cloudLayers, etc.).

## [0.1.1] - 2026-05-10

### CI

- First-ever GitHub Actions workflow coverage — build + test + lint on
  Ubuntu 24.04, build-only on macos-latest, markdown-lint via
  `DavidAnson/markdownlint-cli2-action`
  ([`62f7998`](https://github.com/Reddimus/nws-cpp/commit/62f7998)).
- `build-windows` job added via vcpkg (parity with the rest of the SDK
  family).
- `release.yml` auto-creates a GitHub Release on `vX.Y.Z` tag push,
  with body sourced from this CHANGELOG via `--notes-file` so inline
  ` `code` ` spans survive
  ([`e5103c1`](https://github.com/Reddimus/nws-cpp/commit/e5103c1),
  [`89df4dd`](https://github.com/Reddimus/nws-cpp/commit/89df4dd)).
- Tag/CMakeLists `VERSION` drift is caught at release time
  ([`c2583c8`](https://github.com/Reddimus/nws-cpp/commit/c2583c8)).
- `actions/checkout@v6` upgrade for Node 24 runtime
  ([`e33b8d8`](https://github.com/Reddimus/nws-cpp/commit/e33b8d8)).
- `.markdownlint-cli2.yaml` config disables `MD013` (line-length) and
  other style-noise rules; sibling repos share the same shape.
- `MD004` (asterisk style) disabled for prose continuation parity
  ([`8b73cff`](https://github.com/Reddimus/nws-cpp/commit/8b73cff)).
- `CLAUDE.md` markdown reflowed to satisfy the new lint job
  ([`7cecb0e`](https://github.com/Reddimus/nws-cpp/commit/7cecb0e)).
- `cpp_auto_audit.py` now walks `--cached + --others` so new test
  files don't pass local lint and fail CI
  ([`64d76e9`](https://github.com/Reddimus/nws-cpp/commit/64d76e9)).

### Docs

- Add a Contributing section to the README
  ([`c9a66d2`](https://github.com/Reddimus/nws-cpp/commit/c9a66d2)).
- Add CI / release / C++ standard / license badges to the top of the
  README ([`64b2db3`](https://github.com/Reddimus/nws-cpp/commit/64b2db3)).
- Squash double-blank-line inserted by the Contributing section
  ([`90325de`](https://github.com/Reddimus/nws-cpp/commit/90325de)).
- Add this `CHANGELOG.md` + the auto-release-on-tag workflow it feeds
  ([`e5103c1`](https://github.com/Reddimus/nws-cpp/commit/e5103c1)).

### Build

- Enforce explicit local cpp types ([`b90f909`](https://github.com/Reddimus/nws-cpp/commit/b90f909)).

### Performance

- `-march=x86-64-v3` is the new CMake default when `NATIVE_ARCH` is
  off — gives AVX2/BMI2/FMA without losing portability across modern
  x86 hardware
  ([`7889dcb`](https://github.com/Reddimus/nws-cpp/commit/7889dcb)).

### Chore

- Production-hardening `.gitignore` patterns mirrored from the rest of
  the SDK family
  ([`3a6cd83`](https://github.com/Reddimus/nws-cpp/commit/3a6cd83)).

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

[Unreleased]: https://github.com/Reddimus/nws-cpp/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/Reddimus/nws-cpp/compare/v0.1.1...v0.2.0
[0.1.1]: https://github.com/Reddimus/nws-cpp/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/Reddimus/nws-cpp/releases/tag/v0.1.0
