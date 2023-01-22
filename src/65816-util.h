/**
 * 65(c)816 simulator/emulator (816CE)
 * Copyright (C) 2023 Ray Clemens
 */


#ifndef UTIL_65816_H
#define UTIL_65816_H

#include <stdint.h>
#include <stdbool.h>

#include "65816.h"

// CPU-related helper functions
void _cpu_update_pc(CPU_t *, uint16_t);
uint8_t _cpu_get_sr(CPU_t *);
void _cpu_set_sr(CPU_t *, uint8_t);
void _cpu_set_sp(CPU_t *, uint16_t);
uint32_t _cpu_get_pbr(CPU_t *);
uint32_t _cpu_get_dbr(CPU_t *);
uint32_t _cpu_get_effective_pc(CPU_t *);
void _cpu_update_pc(CPU_t *, uint16_t);
uint8_t _cpu_get_immd_byte(CPU_t *, memory_t *, bool);
uint16_t _cpu_get_immd_word(CPU_t *, memory_t *, bool);
uint32_t _cpu_get_immd_long(CPU_t *, memory_t *, bool);
uint32_t _addr_add_val_page_wrap(uint32_t, uint32_t);
uint32_t _addr_add_val_bank_wrap(uint32_t, uint32_t);
void _cpu_crash(CPU_t *);

// Memory-related functions
// These are THE ONLY functions which should directly
// access data within the memory_t datastructure
uint8_t _get_mem_byte(memory_t *, uint32_t, bool);
uint16_t _get_mem_word(memory_t *, uint32_t, bool);
uint16_t _get_mem_word_bank_wrap(memory_t *, uint32_t, bool);
uint32_t _get_mem_long_bank_wrap(memory_t *, uint32_t, bool);
void _set_mem_byte(memory_t *, uint32_t, uint8_t, bool);
void _set_mem_word(memory_t *, uint32_t, uint16_t, bool);
void _set_mem_word_bank_wrap(memory_t *, uint32_t, uint16_t, bool);
void _init_mem_arr(memory_t *, uint8_t *, uint32_t, uint32_t);
void _save_mem_arr(memory_t *, uint8_t *, uint32_t, uint32_t);
mem_flag_t _test_and_reset_mem_flags(memory_t *, uint32_t, uint8_t);

// CPU-Addressing Modes
void _stackCPU_pushByte(CPU_t *, memory_t *, uint8_t, bool);
void _stackCPU_pushWord(CPU_t *, memory_t *, uint16_t, Emul_Stack_Mod_t, bool);
void _stackCPU_push24(CPU_t *, memory_t *, uint32_t, bool);
uint8_t _stackCPU_popByte(CPU_t *, memory_t *, Emul_Stack_Mod_t, bool);
uint16_t _stackCPU_popWord(CPU_t *, memory_t *, Emul_Stack_Mod_t, bool);
uint32_t _stackCPU_pop24(CPU_t *, memory_t *, bool);

// Get memory (still an address - just indirect)
uint16_t _addrCPU_getAbsoluteIndexedIndirectX(CPU_t *, memory_t *, bool);
uint16_t _addrCPU_getAbsoluteIndirect(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getAbsoluteIndirectLong(CPU_t *, memory_t *, bool);

// Get address
uint32_t _addrCPU_getAbsolute(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getAbsoluteIndexedX(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getAbsoluteIndexedY(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getLongIndexedX(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getDirectPage(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getDirectPageIndirect(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getDirectPageIndirectLong(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getDirectPageIndexedX(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getDirectPageIndexedIndirectX(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getDirectPageIndexedY(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getDirectPageIndirectIndexedY(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getDirectPageIndirectLongIndexedY(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getRelative8(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getRelative16(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getLong(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getImmediate(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getStackRelative(CPU_t *, memory_t *, bool);
uint32_t _addrCPU_getStackRelativeIndirectIndexedY(CPU_t *, memory_t *, bool);

#endif
