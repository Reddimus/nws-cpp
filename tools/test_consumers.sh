#!/usr/bin/env bash
# Verify the installed package and FetchContent defaults without weather API requests.
set -euo pipefail
source_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
scratch_dir=$(mktemp -d "${TMPDIR:-/tmp}/nws-consumers.XXXXXX")
trap 'rm -rf "$scratch_dir"' EXIT
version=$(sed -nE 's/^project\(nws-cpp VERSION ([0-9.]+) LANGUAGES CXX\)$/\1/p' "$source_dir/CMakeLists.txt")
test -n "$version"
cmake -S "$source_dir" -B "$scratch_dir/sdk" -DCMAKE_BUILD_TYPE=Release \
    -DNWS_BUILD_TESTS=ON -DNWS_BUILD_EXAMPLES=OFF -DNWS_ENABLE_LTO=OFF \
    -DCMAKE_INSTALL_PREFIX="$scratch_dir/prefix" "$@"
cmake --build "$scratch_dir/sdk" --target nws_api --parallel 4
cmake --install "$scratch_dir/sdk"
for dependency in glaze gtest gmock; do
    if [ -e "$scratch_dir/prefix/include/$dependency" ]; then
        echo "SDK install leaked $dependency headers" >&2
        exit 1
    fi
done
cmake -S "$source_dir/tests/consumer" -B "$scratch_dir/installed" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$scratch_dir/prefix" \
    -DNWS_EXPECTED_VERSION="$version"
cmake --build "$scratch_dir/installed" --parallel 4
"$scratch_dir/installed/consumer"
cmake -S "$source_dir/tests/consumer" -B "$scratch_dir/fetchcontent" \
    -DCMAKE_BUILD_TYPE=Release -DNWS_SOURCE_DIR="$source_dir" "$@"
cmake --build "$scratch_dir/fetchcontent" --parallel 4
"$scratch_dir/fetchcontent/consumer"
echo "Installed and FetchContent consumers pass for nws-cpp $version"
