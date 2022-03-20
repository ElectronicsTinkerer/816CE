
#ifndef OPS_65816_H
#define OPS_65816_H

// Get the value of a CPU's SR
// NOTE: __CPU__ must be a pointer to a CPU struct
#define CPU_GET_SR(__CPU__) (*(uint8_t *)&((__CPU__)->P))

// Set the value of the CPU's SR
// NOTE: __CPU__ must be a pointer to a CPU struct
#define CPU_SET_SR(__CPU__, __BYTE__) ((*(uint8_t *)&(__CPU__)->P) = (__BYTE__))

// Get the CPU's BANK, shifted to be bits 16..23 of the value
// NOTE: __CPU__ must be a pointer to a CPU struct
#define CPU_GET_DBR_SHIFTED(__CPU__) (((__CPU__)->PBR & 0xff) << 16)

// Get a CPU's 24 bit PC address
// NOTE: __CPU__ must be a pointer to a CPU struct
#define CPU_GET_EFFECTIVE_PC(__CPU__) ((CPU_GET_DBR_SHIFTED(__CPU__)) | ((__CPU__)->PC & 0xffff))

// Update a CPU's PC value by an offset
// Enforces bank wrapping
// NOTE: __CPU__ must be a pointer to a CPU struct
#define CPU_UPDATE_PC16(__CPU__, __OFFSET__) ((__CPU__)->PC = ((__CPU__)->PC + (__OFFSET__)) & 0xffff)

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
