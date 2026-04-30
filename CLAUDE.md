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
- **JSON**: nlohmann/json via FetchContent. Use `json_string()` / `json_int()` helpers from `models/common.hpp` for null-safe extraction.
- **Tests**: GoogleTest via FetchContent. Fixture files in `tests/fixtures/`.

## Conventions

- Code style: `.clang-format` (LLVM base, tabs, 100 cols)
- Namespace: `nws`
- **No `auto`**: Use explicit types. `auto` is only acceptable for iterators, structured bindings (`auto& [key, val]`), and range-for loops (`const auto& x : container`).
- All model `from_json` functions use the null-safe `json_string(j, "key")` helper, NOT `j.value("key", "")` which throws on null values in nlohmann/json v3.
- Models declare `from_json` in headers, implement in `.cpp` files
- Include order: project headers first, then system headers (enforced by clang-format)
