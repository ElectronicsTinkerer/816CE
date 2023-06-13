# 65816 CPU test runner

This is a quick and dirty test runner to exercise the CPU core (found in `/sim/cpu`) and look for bugs. There are two components:
1) A python program which converts JSON-encoded CPU state information (as provided by Tom Harte's amazing [ProcessorTests](https://github.com/TomHarte/ProcessorTests)) into a custom format which is read by 2:
2) A c program which loads in the information from 1 and steps the CPU core. If a decrepency is found from the expected value, the test name and CPU core dump is printed.

To run the test runner, just clone the ProcessorTests repo to a location (currently my home folder) and run the `run.sh` script in this directory

