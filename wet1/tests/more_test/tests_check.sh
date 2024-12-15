 let i=1; while((i<150)); do ../../bp_main ~/Computer_Arch/wet1/tests/more_test/test${i}.trc > ~/Computer_Arch/wet1/tests/more_test/output${i}.out; let i++; done

 let i=1; while((i<150)); do diff -s -q ~/Computer_Arch/wet1/tests/more_test/test${i}.out ~/Computer_Arch/wet1/tests/more_test/output${i}.out; let i++; done


