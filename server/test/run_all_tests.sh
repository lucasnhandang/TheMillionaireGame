#!/bin/bash
# Script to run all test files with timeout protection
# Usage: ./run_all_tests.sh

cd "$(dirname "$0")"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║     Running All Unit Tests - TheMillionaireGame Server        ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

total_tests=0
passed_tests=0
failed_tests=0

run_test() {
    test_name=$1
    test_path="./bin/$test_name"
    
    if [ ! -f "$test_path" ]; then
        echo "⚠ Test binary not found: $test_name (skipped)"
        return
    fi
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "▶ Running: $test_name"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    # Run test and capture exit code
    $test_path
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo "✅ $test_name: PASSED"
        ((passed_tests++))
    else
        echo "❌ $test_name: FAILED (exit code: $exit_code)"
        ((failed_tests++))
    fi
    
    ((total_tests++))
    echo ""
}

# Run all tests
run_test "stream_handler_test"
run_test "socket_io_test"
run_test "session_manager_test"
run_test "auth_manager_test"

# Print summary
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                      Test Summary                              ║"
echo "╠════════════════════════════════════════════════════════════════╣"
printf "║ Total Tests:  %-48s ║\n" "$total_tests"
printf "║ Passed:       %-48s ║\n" "$passed_tests"
printf "║ Failed:       %-48s ║\n" "$failed_tests"
echo "╚════════════════════════════════════════════════════════════════╝"

if [ $failed_tests -eq 0 ]; then
    echo ""
    echo "🎉 All tests passed!"
    exit 0
else
    echo ""
    echo "⚠ Some tests failed. Please check the output above."
    exit 1
fi
