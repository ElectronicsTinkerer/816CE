
#ifndef OPS_65816_H
#define OPS_65816_H

#include "65816.h"

static void _stackCPU_pushByte(CPU_t *, memory_t *, int32_t);
static void _stackCPU_pushWord(CPU_t *, memory_t *, int32_t, Emul_Stack_Mod_t);
static void _stackCPU_push24(CPU_t *, memory_t *, int32_t);
static int32_t _stackCPU_popByte(CPU_t *, memory_t *, Emul_Stack_Mod_t);
static int32_t _stackCPU_popWord(CPU_t *, memory_t *, Emul_Stack_Mod_t);
static int32_t _stackCPU_pop24(CPU_t *, memory_t *);

static int32_t _addrCPU_getAbsoluteIndexedIndirectX(CPU_t *, memory_t *);
static int32_t _addrCPU_getAbsoluteIndirect(CPU_t *, memory_t *);
static int32_t _addrCPU_getAbsoluteIndirectLong(CPU_t *, memory_t *);
static int32_t _addrCPU_getAbsolute(CPU_t *, memory_t *);
static int32_t _addrCPU_getAbsoluteIndexedX(CPU_t *, memory_t *);
static int32_t _addrCPU_getAbsoluteIndexedY(CPU_t *, memory_t *);
static int32_t _addrCPU_getLongIndexedX(CPU_t *, memory_t *);
static int32_t _addrCPU_getDirectPage(CPU_t *, memory_t *);
static int32_t _addrCPU_getDirectPageIndexedX(CPU_t *, memory_t *);
static int32_t _addrCPU_getDirectPageIndexedY(CPU_t *, memory_t *);
static int32_t _addrCPU_getDirectPageIndirect(CPU_t *, memory_t *);
static int32_t _addrCPU_getDirectPageIndirectLong(CPU_t *, memory_t *);
static int32_t _addrCPU_getRelative8(CPU_t *, memory_t *);
static int32_t _addrCPU_getRelative16(CPU_t *, memory_t *);
static int32_t _addrCPU_getLong(CPU_t *, memory_t *);

#endif
