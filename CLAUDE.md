# nws-cpp Development Guide

## Build & Test

```bash
make build          # Release build (CMake + make)
make debug          # Debug build
make test           # Run unit tests (ctest)
make lint           # Check formatting (clang-format --dry-run)
make format         # Format in place
make coverage       # lcov coverage report
make clean          # Remove build/
```

## Architecture

- **Layered static libraries**: nws_core -> nws_http -> nws_models -> nws_api -> nws (INTERFACE)
- **C++23**: `std::expected<T, Error>` for all returns, no exceptions
- **Patterns**: Pimpl (HttpClient, NWSClient), non-copyable/movable clients, `[[nodiscard]]`
- **JSON**: [Glaze](https://github.com/stephenberry/glaze) v7.6.0 via FetchContent (compile-time reflection, ~3.4x parse speedup over nlohmann on the gridpoint-forecast hot path — migrated 2026-05-11). The migration uses a `glz::generic` AST + hand-walk strategy (see `src/models/glaze_detail.hpp`) because the NWS payload mixes JSON-LD keys (`@id`, `@type`), polymorphic GeoJSON geometry variants, and mixed-shape fields (`ForecastPeriod.windSpeed` is string-or-object) that don't map cleanly onto a static `glz::meta`. See `tests/parse_benchmark.cpp` for the regression guard.
- **Tests**: GoogleTest via FetchContent. Fixture files in `tests/fixtures/`.

## Conventions

- Code style: `.clang-format` (LLVM base, tabs, 100 cols)
- Namespace: `nws`
- **No `auto`**: Use explicit types. `auto` is only acceptable for iterators, structured bindings (`auto& [key, val]`), and range-for loops (`const auto& x : container`).
- Null-safety: all `detail::get_*` helpers in `src/models/glaze_detail.hpp` treat missing / null / type-mismatched JSON values as default-constructed output. Use those instead of inlining `glz::generic` lookups so the per-field error-handling stays uniform.
- Models declare `deserialize_*(std::string_view, T&)` in headers, implement in `.cpp` files. The previous `from_json(const nlohmann::json&, T&)` overloads have been removed.
- Include order: project headers first, then system headers (enforced by clang-format)

## CI

GitHub Actions workflow `.github/workflows/ci.yml`: build + test + lint on Ubuntu 24.04, build + test + lint on macos-latest, build + test on windows-latest (no clang-format step — Windows toolchain lacks a uniform clang-format), markdown-lint via DavidAnson. Release workflow auto-creates a GitHub Release on `vX.Y.Z` tag push (notes extracted from `CHANGELOG.md`).
