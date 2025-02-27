#!/bin/bash

# Directory where test files are located
TEST_DIR="tests"

# Number of passed and failed tests
PASSED=0
FAILED=0

# Find all test command files and iterate over them
for COMMAND_FILE in ${TEST_DIR}/*.command; do
    TEST_NAME=$(basename "${COMMAND_FILE}" .command)
    INPUT_FILE="${TEST_DIR}/${TEST_NAME}.in"
    EXPECTED_OUTPUT_FILE="${TEST_DIR}/${TEST_NAME}.out"
    YOUR_OUTPUT_FILE="${TEST_DIR}/${TEST_NAME}.YoursOut"

    # Read the command to run the program from the command file
    COMMAND=$(cat "${COMMAND_FILE}")

    # Run the program with the input file and capture the output
    ${COMMAND} < "${INPUT_FILE}" > "${YOUR_OUTPUT_FILE}"

    # Compare the output with the expected output
    if diff -q "${EXPECTED_OUTPUT_FILE}" "${YOUR_OUTPUT_FILE}" > /dev/null; then
        echo "${TEST_NAME}: PASSED"
        PASSED=$((PASSED + 1))
    else
        echo "${TEST_NAME}: FAILED"
        FAILED=$((FAILED + 1))
    fi
done

# Print summary
echo "Total Passed: ${PASSED}"
echo "Total Failed: ${FAILED}"

# Exit with the number of failed tests as the status code
exit ${FAILED}
