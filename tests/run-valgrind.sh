#!/bin/sh
# Build every smoke test and run each executable directly under Valgrind.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build-valgrind}"
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

command -v valgrind >/dev/null 2>&1 || {
    echo "valgrind is required" >&2
    exit 1
}

cmake -S "$ROOT" -B "$BUILD" \
    -DDIRECTGATE_BUILD_TESTS=ON \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$BUILD" -j "$JOBS"

TEST_LIST="$BUILD/valgrind-tests.txt"
find "$BUILD/tests" -maxdepth 1 -type f -perm -111 -name '*_smoke' -print | sort > "$TEST_LIST"
[ -s "$TEST_LIST" ] || {
    echo "no smoke test executables found" >&2
    exit 1
}

while IFS= read -r test; do
    echo "Valgrind: $(basename "$test")"
    valgrind \
        --leak-check=full \
        --show-leak-kinds=definite,indirect,possible \
        --errors-for-leak-kinds=definite,indirect,possible \
        --track-origins=yes \
        --error-exitcode=1 \
        "$test"
done < "$TEST_LIST"
