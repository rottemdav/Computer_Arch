let i=1
num=0

# First loop to generate output files
while ((i < 150)); do 
    ../../bp_main ~/Computer_Arch/wet1/tests/more_test/test${i}.trc > ~/Computer_Arch/wet1/tests/more_test/output${i}.out
    let i++
done

# Reset i for the second loop
let i=1

# Second loop to compare and count differences
while ((i < 150)); do
    diff_result=$(diff -s -q ~/Computer_Arch/wet1/tests/more_test/test${i}.out ~/Computer_Arch/wet1/tests/more_test/output${i}.out)

    if [[ $diff_result == *"differ"* ]]; then
        echo -n "Difference found in file test${i}.trc: "
        head -n 1 ~/Computer_Arch/wet1/tests/more_test/test${i}.trc
        num=$((num + 1)) # Correct syntax to increment num
    fi
    let i++
done

# Print the total number of differing tests
echo "Total number of differing tests: ${num}"
