#!/bin/bash
echo "Executable: $1"
for test in *.in; do
    echo "Testing $test"
    $1 "$test" | diff - "${test%.in}.out"
    exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo "Test $test failed with exit code $exit_code"
        exit $exit_code
    fi
done