#!/bin/bash -e
./json2cpu.py ~/ProcessorTests/65816/v1/*
cc runner.c ../src/cpu/*.c && ./a.out output.scpu

