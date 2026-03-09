#!/bin/bash
set -e

# Define directories (run from project root or any dir; script dir = project root)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${PROJECT_ROOT:-$SCRIPT_DIR}"
BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build_tests}"
COVERAGE_DIR="${COVERAGE_DIR:-$PROJECT_ROOT/coverage_report}"

# Threshold
MIN_COVERAGE=85

echo "=========================================="
echo " Setting up Unit Tests and Code Coverage "
echo "=========================================="

# 1. Configure CMake with Coverage Flags
echo "[1/4] Configuring build..."
rm -rf "$BUILD_DIR"
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug

# 2. Build the tests
echo "[2/4] Building tests..."
cmake --build "$BUILD_DIR" -j$(nproc)

# 3. Run the tests
echo "[3/4] Running tests..."
cd "$BUILD_DIR"
ctest --output-on-failure
cd "$PROJECT_ROOT"

# 4. Generate Coverage Report
echo "[4/4] Generating Coverage Report..."
rm -rf "$COVERAGE_DIR"

# Capture initial coverage
lcov --capture --directory "$BUILD_DIR" --output-file "$BUILD_DIR/coverage.info" --rc lcov_branch_coverage=1

# Filter out system headers, gtest, test files
lcov --remove "$BUILD_DIR/coverage.info" \
    '/usr/*' \
    '*/tests/*' \
    '*_deps/*' \
    '*/mocks/*' \
    '*/cmake/*' \
    --output-file "$BUILD_DIR/coverage_filtered.info" --rc lcov_branch_coverage=1

# Generate HTML
genhtml "$BUILD_DIR/coverage_filtered.info" --output-directory "$COVERAGE_DIR" --rc lcov_branch_coverage=1 > "$BUILD_DIR/genhtml_output.txt"

echo "=========================================="
echo " Coverage Summary"
echo "=========================================="

# Use lcov --summary for a nice overview
lcov --summary "$BUILD_DIR/coverage_filtered.info"

echo "-------------------------------------------------"
# Extract total coverage from lcov summary
TOTAL_COV=$(lcov --summary "$BUILD_DIR/coverage_filtered.info" 2>&1 | grep "lines......:" | awk '{print $2}' | sed 's/%//g')
echo "Total Project Coverage: $TOTAL_COV%"
echo "-------------------------------------------------"

TOTAL_COV_INT=$(printf "%.0f\n" "$TOTAL_COV")

if [ "$TOTAL_COV_INT" -lt "$MIN_COVERAGE" ]; then
    echo "❌ Build failed: Test coverage ($TOTAL_COV_INT%) is below the minimum threshold of $MIN_COVERAGE%!"
    exit 1
else
    echo "✅ Coverage check passed."
fi

echo "Detailed HTML report: file://$COVERAGE_DIR/index.html"
exit 0
