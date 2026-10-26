# NWS C++ SDK - Root Makefile
# Wraps CMake for convenient day-to-day workflow

BUILD_DIR ?= build
BUILD_TYPE ?= Release
CMAKE_ARGS ?=
CPP_AUTO_AUDIT := python3 tools/cpp_auto_audit.py
CMAKE := cmake
NPROC := $(shell nproc 2>/dev/null || echo 4)

.PHONY: all build debug test lint clean configure help bench format pre-commit install-hooks coverage consumers \
	run-basic_usage run-get_forecast run-get_observations run-get_alerts run-weather_monitor

# Default target
all: build

# Configure CMake (if needed)
configure:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && $(CMAKE) .. -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_ARGS)

# Configure a separate Debug build.
configure-debug:
	@$(MAKE) configure BUILD_DIR=build-debug BUILD_TYPE=Debug

# Build all targets
build: configure
	@$(CMAKE) --build $(BUILD_DIR) -j$(NPROC)

# Debug build
debug:
	@$(MAKE) build BUILD_DIR=build-debug BUILD_TYPE=Debug

# Run tests
test: build
	@cd $(BUILD_DIR) && ctest --output-on-failure

# Run linting (clang-format check)
lint:
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "Checking code formatting..."; \
		find src include tests examples \( -name '*.cpp' -o -name '*.hpp' \) -print0 | \
			xargs -0 clang-format --dry-run --Werror && \
		echo "Format check passed."; \
	else \
		echo "clang-format not found. Install clang-format to run lint."; \
		exit 1; \
	fi
	$(CPP_AUTO_AUDIT)
	python3 tools/test_install_hooks.py


# Format code in place
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "Formatting code..."; \
		find src include tests examples \( -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format -i; \
		echo "Done"; \
	else \
		echo "clang-format not found. Install clang-format to format code."; \
		exit 1; \
	fi

# Code coverage (requires lcov)
coverage:
	@mkdir -p build-coverage
	@cd build-coverage && $(CMAKE) .. -DCMAKE_BUILD_TYPE=Debug -DNWS_ENABLE_COVERAGE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	@$(CMAKE) --build build-coverage -j$(NPROC)
	@cd build-coverage && ctest --output-on-failure
	@lcov --capture --directory build-coverage --output-file build-coverage/coverage.info --ignore-errors mismatch
	@lcov --remove build-coverage/coverage.info '/usr/*' '*/build-coverage/_deps/*' --output-file build-coverage/coverage_filtered.info --ignore-errors unused
	@genhtml build-coverage/coverage_filtered.info --output-directory build-coverage/coverage-report
	@echo "Coverage report: build-coverage/coverage-report/index.html"

# pre-commit: auto-format, then lint. Run before every commit to avoid
# the recurring "push -> CI clang-format/auto-audit fail -> follow-up fix
# PR" loop. Idempotent — `format` re-running is a no-op.
pre-commit: format lint

# Installs a lint-only hook; preserves custom hooks and works in worktrees.
install-hooks:
	@hook="$$(git rev-parse --git-path hooks)/pre-commit"; \
	if [ -f "$$hook" ]; then \
        case "$$(cat "$$hook")" in \
            "$$(printf '#!/bin/sh\nexec make lint')"|"$$(printf '#!/bin/sh\nexec make pre-commit')") ;; \
            *) echo "$$hook already exists; not replacing it"; exit 1 ;; \
        esac; \
    fi; \
	mkdir -p "$$(dirname "$$hook")" && printf '#!/bin/sh\nexec make lint\n' > "$$hook" && \
	chmod +x "$$hook" && echo "Installed $$hook (runs make lint)"


# Clean build artifacts
clean:
	@rm -rf $(BUILD_DIR) build-coverage
	@echo "Cleaned build directory"

# Run benchmark
bench: build
	@BUILD_DIR="$(BUILD_DIR)" ./tools/bench.sh

consumers:
	@./tools/test_consumers.sh

# Run examples
run-basic_usage: build
	@./$(BUILD_DIR)/examples/example_basic

run-get_forecast: build
	@./$(BUILD_DIR)/examples/example_forecast

run-get_observations: build
	@./$(BUILD_DIR)/examples/example_observations

run-get_alerts: build
	@./$(BUILD_DIR)/examples/example_alerts

run-weather_monitor: build
	@./$(BUILD_DIR)/examples/example_monitor

# Help
help:
	@echo "NWS C++ SDK Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make build           - Configure and build the SDK (Release)"
	@echo "  make debug           - Configure and build the SDK (Debug)"
	@echo "  make test            - Run tests"
	@echo "  make bench           - Run the offline parsing benchmark"
	@echo "  make lint            - Check code formatting"
	@echo "  make format          - Format code in place"
	@echo "  make coverage        - Generate code coverage report (requires lcov)"
	@echo "  make clean           - Remove build artifacts"
	@echo "  make help            - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make run-basic_usage     - Run basic usage example"
	@echo "  make run-get_forecast    - Run forecast example"
	@echo "  make run-get_observations- Run observations example"
	@echo "  make run-get_alerts      - Run alerts example"
	@echo "  make run-weather_monitor - Run weather monitor example"
	@echo ""
	@echo "Variables:"
