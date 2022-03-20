

#ifndef UTIL_65816_H
#define UTIL_65816_H

#include <stdint.h>
#include "65816.h"

// CPU-related functions
uint8_t _cpu_get_sr(CPU_t *);
void _cpu_set_sr(CPU_t *, uint8_t);
uint32_t _cpu_get_pbr(CPU_t *);
uint32_t _cpu_get_effective_pc(CPU_t *);
void _cpu_update_pc(CPU_t *, uint16_t);
uint8_t _cpu_get_immd_byte(CPU_t *, memory_t *);
uint16_t _cpu_get_immd_word(CPU_t *, memory_t *);
uint32_t _addr_add_val_page_wrap(uint32_t, uint32_t);
uint32_t _addr_add_val_bank_wrap(uint32_t, uint32_t);
uint16_t _get_mem_word_bank_wrap(memory_t *, uint32_t);

// Memory-related functions
// These are THE ONLY functions which should directly
// access data within the memory_t datastructure
uint8_t _get_mem_byte(memory_t *, uint32_t);
uint16_t _get_mem_word(memory_t *, uint32_t);

#endif
