

#ifndef CPU_65816_H
#define CPU_65816_H

#include <stdint.h>

// Interrupt Vectors
#define CPU_VEC_NATIVE_COP 0xffe4
#define CPU_VEC_NATIVE_BRK 0xffe6
#define CPU_VEC_NATIVE_ABORT 0xffe8
#define CPU_VEC_NATIVE_NMI 0xffea
#define CPU_VEC_NATIVE_IRQ 0xffee
#define CPU_VEC_EMU_COP 0xfff4
#define CPU_VEC_EMU_ABORT 0xfff8
#define CPU_VEC_EMU_NMI 0xfffa
#define CPU_VEC_RESET 0xfffc
#define CPU_VEC_EMU_IRQ 0xfffe

// CPU "Class"
typedef struct CPU CPU;

struct CPU 
{
    int32_t C;
    int32_t DBR;
    int32_t X;
    int32_t Y;
    int32_t D;
    int32_t SP;
    int32_t PBR;
    int32_t PC;
    struct {
        unsigned char C : 1;   
        unsigned char Z : 1;
        unsigned char I : 1;
        unsigned char D : 1;
        unsigned char X : 1;
        unsigned char M : 1;
        unsigned char V : 1;
        unsigned char N : 1;
        unsigned char E : 1;
        unsigned char RST : 1; // 1 if the CPU was reset, 0 if reset vector has been jumped to
        unsigned char IRQ : 1; // 1 if IRQ input is asserted, 0 else
        unsigned char NMI : 1; // 1 if NMI input is asserted, 0 else
        // unsigned char ABT : 1; // 1 if ABORT input is asserted, 0 else
    } P;
};

void resetCPU(CPU*);
void stepCPU(CPU*, int16_t*);
static void stackCPU_pushByte(CPU *, int16_t *mem, int32_t);
static void stackCPU_pushWord(CPU *, int16_t *mem, int32_t);
static int32_t stackCPU_popByte(CPU *, int16_t *mem);
static int32_t stackCPU_popWord(CPU *, int16_t *mem);


#endif
