#!/bin/bash
for filename in CompArchHw1Tests/test*.trc; do
    test_num=$(echo "$filename" | cut -d'.' -f1)
    dos2unix "$filename"
    ./bp_main "$filename" > "${test_num}Yours.out"
    echo "Comparing $test_num.out"
    diff "${test_num}.out" "${test_num}Yours.out"
done