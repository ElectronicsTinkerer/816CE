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
    CPU_t cpu_run, cpu_final;
    char cpu_state_run[1000];
    char cpu_state_final[1000];
    char *str = NULL;

    size_t len = 0;
    long test_num = 0;
    size_t id_idx = 0, fd_idx = 0;

    memory_t *mem = calloc(16777216, sizeof(*mem));

    printf("Running.\n");
    
    while (1) {
        if (getline(&test_name, &len, fp) == -1) {
            printf("End of file. Tests run: %ld\n", test_num);
            break;
        }

        getline(&str, &len, fp);
        id_idx = 0;
        while (str[0] == 'i') {
            sscanf(str, "i: %x : %hhx\n", &(input_data[id_idx].addr), &(input_data[id_idx].data));
            // free(str);
            _set_mem_byte(mem, input_data[id_idx].addr, input_data[id_idx].data, false);
            ++id_idx;
            getline(&str, &len, fp);
        }
        fd_idx = 0;
        while (str[0] == 'f') {
            sscanf(str, "f: %x : %hhx\n", &(output_data[fd_idx].addr), &(output_data[fd_idx].data));
            // free(str);
            _set_mem_byte(mem, output_data[fd_idx].addr, output_data[fd_idx].data, false);
            ++fd_idx;
            getline(&str, &len, fp);
        }
        cpu_istr = str;
        
        // Scan cpu expected output
        getline(&cpu_fstr, &len, fp);

        // set up CPU
        memset(&cpu_run, 0, sizeof(cpu_run));
        memset(&cpu_final, 0, sizeof(cpu_final));
        fromstrCPU(&cpu_run, cpu_istr);
        fromstrCPU(&cpu_final, cpu_fstr);
        stepCPU(&cpu_run, mem);

        if (memcmp(&cpu_run, &cpu_final, sizeof(cpu_run)) != 0) {
            printf("Test failed! (%ld) : %s", test_num+1, test_name);
            tostrCPU(&cpu_run, cpu_state_run);
            tostrCPU(&cpu_final, cpu_state_final);
            printf("RUN CPU: '%s'\n", cpu_state_run);
            printf("FIN CPU: '%s'\n", cpu_state_final);
        }

        // free(cpu_istr);
        // free(cpu_fstr);

        // Reset memory
        while (id_idx-- > 0) {
            _set_mem_byte(mem, input_data[id_idx].addr, input_data[id_idx].data, false);
        }
        while (fd_idx-- > 0) {
            _set_mem_byte(mem, output_data[fd_idx].addr, output_data[fd_idx].data, false);
        }

        // for (long i = 0; i < 1000000000; i++) {
        // }

        test_num += 1;

        if (test_num & 0xffff == 0) {
            printf("%ld\n", test_num);
        }
    }

    free(mem);
    
    fclose(fp);
}

