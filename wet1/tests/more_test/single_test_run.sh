#!/bin/bash
number=14
head -n 1 test${number}.trc
 ../../bp_main test${number}.trc > output${number}.out
 echo "comparing..."
 diff output${number}.out test${number}.out
 echo "done comparing!"