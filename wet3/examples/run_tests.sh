#!/bin/bash
#cmod +x ./run_tests.sh

echo Running tests...
echo The test succeed if there are no diffs printed.
echo

for filename in tests/test*.command; do
    test_num=`echo $filename | cut -d'.' -f1`
    bash ${filename} > ${test_num}.YoursOut
done

for filename in tests/test*.out; do
    test_num=`echo $filename | cut -d'.' -f1`
    diff_result=$(diff ${test_num}.out ${test_num}.YoursOut)
    if [ "$diff_result" != "" ]; then
        echo The test ${test_num} didnt pass!
    fi
done

echo
echo Ran all tests.
