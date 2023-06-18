/**
 * 65(c)816 simulator/emulator (816CE)
 * Copyright (C) 2023 Ray Clemens
 *
 * "Quick'NDirty" Test Runner
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../src/cpu/65816.h"
#include "../src/cpu/65816-util.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Single file expected.\n");
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        printf("Unable to open file.\n");
        exit(-1);
    }

    char *test_name = NULL;
    struct {
        uint32_t addr;
        uint8_t data;
    } input_data[100], output_data[100];
    char *cpu_istr = NULL;
    char *cpu_fstr = NULL;
    CPU_t cpu_initial, cpu_run, cpu_final;
    char cpu_state[1000];
    char *str = NULL;

    size_t len = 0;
    long test_num = 0, success_count = 0;
    size_t id_idx = 0, fd_idx = 0, i;
    bool failed = false;
    uint8_t byte_val;

    memory_t *mem = calloc(16777216, sizeof(*mem));

    printf("Running.\n");
    
    while (1) {
        if (getline(&test_name, &len, fp) == -1) {
            printf("End of file. Tests passed: %ld/%ld\n", success_count, test_num);
            break;
        }

        failed = false;

        getline(&str, &len, fp);
        id_idx = 0;
        while (str[0] == 'i') {
            sscanf(str, "i: %x : %hhx\n", &(input_data[id_idx].addr), &(input_data[id_idx].data));
            _set_mem_byte(mem, input_data[id_idx].addr, input_data[id_idx].data, false);
            ++id_idx;
            getline(&str, &len, fp);
        }
        fd_idx = 0;
        while (str[0] == 'f') {
            sscanf(str, "f: %x : %hhx\n", &(output_data[fd_idx].addr), &(output_data[fd_idx].data));
            ++fd_idx;
            getline(&str, &len, fp);
        }
        cpu_istr = str;
        
        // Scan cpu expected output
        getline(&cpu_fstr, &len, fp);

        // set up CPU
        memset(&cpu_initial, 0, sizeof(cpu_initial));
        memset(&cpu_final, 0, sizeof(cpu_final));
        fromstrCPU(&cpu_initial, cpu_istr);
        memcpy(&cpu_run, &cpu_initial, sizeof(cpu_run));
        fromstrCPU(&cpu_final, cpu_fstr);
        stepCPU(&cpu_run, mem);

        if (memcmp(&cpu_run, &cpu_final, sizeof(cpu_run)) != 0) {
            failed = true;
        }
        else {
            ++success_count;
        }

        // Check memory
        i = fd_idx;
        while (i-- > 0) {
            // Check for memory modifications
            if (output_data[i].data != _get_mem_byte(mem, output_data[i].addr, false)) {
                failed = true;
            }
        }

        if (failed) {
            printf("Test failed! (%ld) : %s", test_num+1, test_name);
            for (i = 0; i < id_idx; ++i) {
                printf("i:%06x:%02x\n", input_data[i].addr, input_data[i].data);
            }
            for (i = 0; i < fd_idx; ++i) {
                printf("f:%06x:%02x", output_data[i].addr, output_data[i].data);
                byte_val = _get_mem_byte(mem, output_data[i].addr, false);
                if (output_data[i].data    != byte_val) {
                    printf(" (actual: %02x)", byte_val);
                }
                printf("\n");
            }
            tostrCPU(&cpu_initial, cpu_state);
            printf("INITIAL  CPU: '%s'\n", cpu_state);
            tostrCPU(&cpu_run, cpu_state);
            printf("ACTUAL   CPU: '%s'\n", cpu_state);
            tostrCPU(&cpu_final, cpu_state);
            printf("EXPECTED CPU: '%s'\n", cpu_state);
        }

        // Reset memory
        while (fd_idx-- > 0) {
            _set_mem_byte(mem, output_data[fd_idx].addr, output_data[fd_idx].data, false);
        }
        while (id_idx-- > 0) {
            _set_mem_byte(mem, input_data[id_idx].addr, input_data[id_idx].data, false);
        }

        test_num += 1;

        if (test_num & 0xffff == 0) {
            printf("%ld\n", test_num);
        }
    }

    free(mem);
    
    fclose(fp);
}

