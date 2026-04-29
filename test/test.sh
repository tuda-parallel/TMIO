#!/bin/bash

GIT_REPO=$(readlink -f ..)
cd "$GIT_REPO/build" || exit 1

# Colors
BLACK="\033[0m"
GREEN="\033[1;32m"
RED="\033[1;31m"

# Array to store failed tests
FAILED_TESTS=()

# Function to run a test and report result
run_test() {
    local num="$1"
    local target="$2"

    if make "$target" 1>/dev/null; then
        echo -e "${GREEN}passed test (${num}/10): ${target}${BLACK}"
    else
        echo -e "${RED}failed test (${num}/10): ${target}${BLACK}"
        FAILED_TESTS+=("$target")
    fi
}

# Standard tests
run_test 0 clean
run_test 1 build
run_test 2 run
run_test 3 msgpack

# Library tests
make clean
run_test 4 library
run_test 5 msgpack_library

# Special tests
run_test 6 debug
run_test 7 dhat
run_test 8 memory_overhead
run_test 9 openmp

# Python tools
run_test 10 tools

# Clean up
make clean

# Summary
if [ ${#FAILED_TESTS[@]} -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${BLACK}"
else
    echo -e "${RED}Failed tests summary:${BLACK}"
    for t in "${FAILED_TESTS[@]}"; do
        echo -e "  - $t"
    done
fi
