#!/bin/bash -e
if [ ! -f output.scpu ]; then
    ./json2cpu.py ~/ProcessorTests/65816/v1/*
fi
cc -g runner.c ../src/cpu/*.c && ./a.out output.scpu

