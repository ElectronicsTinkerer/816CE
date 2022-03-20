

#ifndef UTIL_65816_H
#define UTIL_65816_H

#include "65816.h"

uint8_t _cpu_get_sr(CPU_t *);
void _cpu_set_sr(CPU_t *, uint8_t);
uint32_t _cpu_get_dbr(CPU_t *, uint8_t);
uint32_t _cpu_get_effective_pc(CPU_t *);
void _cpu_update_pc(CPU_t *cpu, uint16_t offset);

// Get a byte from memory
// NOTE: __MEM__ must be a pointer to an int16_t array
#define ADDR_GET_MEM_BYTE(__MEM__, __ADDR__) ((__MEM__)[__ADDR__] & 0xff)

// Get the byte stored in memory at a CPU's PC+1
// NOTE: __CPU__ must be a pointer to a CPU struct
#define ADDR_GET_MEM_IMMD_BYTE(__CPU__, __MEM__) (ADDR_GET_MEM_BYTE(__MEM__, ADDR_ADD_VAL_BANK_WRAP(CPU_GET_EFFECTIVE_PC(__CPU__), 1)))

// Get the word stored in memory at a CPU's PC+1 and PC+2
// NOTE: __CPU__ must be a pointer to a CPU struct
#define ADDR_GET_MEM_IMMD_WORD(__CPU__, __MEM__) (ADDR_GET_MEM_IMMD_BYTE(__CPU__, __MEM__) | (ADDR_GET_MEM_BYTE(__MEM__, ADDR_ADD_VAL_BANK_WRAP(CPU_GET_EFFECTIVE_PC(__CPU__), 2)) << 8))

// Add a value to an address, wrapping around the page if necessary
#define ADDR_ADD_VAL_PAGE_WRAP(__ADDR__, __OFFSET__) (((__ADDR__)&0xffff00) | (((__ADDR__) + (__OFFSET__)) & 0xff))

// Add a value to an address, wrapping around the bank if necessary
#define ADDR_ADD_VAL_BANK_WRAP(__ADDR__, __OFFSET__) (((__ADDR__)&0xff0000) | (((__ADDR__) + (__OFFSET__)) & 0xffff))

// Get a word in memory with no wrapping.
#define ADDR_GET_MEM_ABS_WORD(__MEM__, __ADDR__) ((ADDR_GET_MEM_BYTE(__MEM__, __ADDR__) | (ADDR_GET_MEM_BYTE(__MEM__, __ADDR__ + 1) << 8)) & 0xffff)

// Get a word in memory with bank wrapping.
#define ADDR_GET_MEM_DP_WORD(__MEM__, __ADDR__) ((ADDR_GET_MEM_BYTE(__MEM__, __ADDR__) | (ADDR_ADD_VAL_BANK_WRAP(ADDR_GET_MEM_BYTE(__MEM__, __ADDR__), 1) << 8)) & 0xffff)

#endif

