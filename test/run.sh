#!/bin/bash
cc runner.c ../src/cpu/*.c

TEST_DIR="op_tests"
RSLT_DIR="results"

if [ ! -d "$TEST_DIR" ] ; then
    mkdir -p "$TEST_DIR"
fi
if [ ! -d "$RSLT_DIR" ] ; then
    mkdir -p "$RSLT_DIR"
fi


run_test() {
    echo "$1"
    TEST_FILE="$TEST_DIR/$1.scpu"
    RSLT_FILE="$RSLT_DIR/$1.out"

    if [ ! -f "$TEST_FILE" ]; then
        ./json2cpu.py "$TEST_FILE" ~/ProcessorTests/65816/v1/"$1".{e,n}.json
    fi

    ./a.out "$TEST_FILE" > "$RSLT_FILE"
}

if [ $# -eq 0 ]; then
    for i in {0,1,2,3,4,5,6,7,8,9,a,b,c,d,e,f}{0,1,2,3,4,5,6,7,8,9,a,b,c,d,e,f}; do
        run_test "$i" &
    done
else
    run_test "$1" &
fi


wait

echo "Complete"
