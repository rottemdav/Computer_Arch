#!/bin/bash
for command_file in tests/test*.command; do
    # Extract the base filename without extension
    test_num=$(basename "$command_file" .command)
    
    # Convert to Unix format, suppressing any errors or output
    #dos2unix "$command_file" 2>/dev/null
    
    # Execute the command in the .command file and direct output to .MyOut
    ./"$command_file" > "tests/${test_num}.MyOut"
    
    # Compare the output files
    if diff --suppress-common-lines --side-by-side "tests/${test_num}.YoursOut" "tests/${test_num}.MyOut"; then
        echo "Success: ${test_num}.MyOut matches ${test_num}.YoursOut"
    else
        echo "Differences found for ${test_num}:"
        diff --side-by-side "tests/${test_num}.YoursOut" "tests/${test_num}.MyOut"
    fi
done
