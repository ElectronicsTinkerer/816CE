

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
typedef struct CPU_t CPU_t;

struct CPU_t 
{
    uint16_t C;
    uint8_t  DBR;
    uint16_t X;
    uint16_t Y;
    uint16_t D;
    uint16_t SP;
    uint8_t  PBR;
    uint16_t PC;
    struct {
        // Order matters (keep in sync with SR in CPU):
        unsigned char C : 1;   
        unsigned char Z : 1;
        unsigned char I : 1;
        unsigned char D : 1;
        unsigned char XB : 1; // B in emulation
        unsigned char M : 1;
        unsigned char V : 1;
        unsigned char N : 1;
        // Order no longer matters:
        unsigned char E : 1;
        unsigned char RST : 1; // 1 if the CPU was reset, 0 if reset vector has been jumped to
        unsigned char IRQ : 1; // 1 if IRQ input is asserted, 0 else
        unsigned char NMI : 1; // 1 if NMI input is asserted, 0 else
        unsigned char STP : 1; // 1 if CPU has executed a STP instruction, 0 else
        // unsigned char ABT : 1; // 1 if ABORT input is asserted, 0 else
    } P;

    // Total phi-1 cycles the CPU has run
    // Just prepairing for the end of the Universe ... don't worry about it :)
    uint64_t cycles;
};

// Possible error codes from CPU public (non-static) functions
typedef enum CPU_Error_Code_t
 {
     CPU_ERR_OK = 0,
     CPU_ERR_UNKNOWN_OPCODE,
     CPU_ERR_STP, // Returned if stepCPU() is called on a CPU which is in the SToPped state due to a STP instruction
     CPU_ERR_NULL_CPU, // Only used if `CPU_DEBUG_CHECK_NULL` is defined
 } CPU_Error_Code_t;

// Used to specify if the call to stack operations should allow
// keeping the stack within page 1 while a CPU is in emulation mode
typedef enum Emul_Stack_Mod_t
 {
     CPU_ESTACK_DISABLE = 0,
     CPU_ESTACK_ENABLE
 } Emul_Stack_Mod_t;

 // Specifies the addressing mode for use when performing an operation
typedef enum CPU_Addr_Mode_t
{
    CPU_ADDR_DP = 0,  // Direct page -> dp
    CPU_ADDR_DPX,     // Direct page indexed X -> dp,X
    CPU_ADDR_DPY,     // Direct page indexed Y -> dp,Y
    CPU_ADDR_ABS,     // Absolute -> abs
    CPU_ADDR_ABSX,    // Absolute indexed X -> abs,X
    CPU_ADDR_INDABS,  // Indirect absolute -> (abs)
    CPU_ADDR_ABSL,    // Absolute Long -> abs_long
    CPU_ADDR_ABSINDL, // Absolute Indirect Long -> [abs]
    CPU_ADDR_ABSINDX, // Absolute Indirect Indexed X -> (abs,X)
} CPU_Addr_Mode_t;

typedef uint8_t memory_t;

CPU_Error_Code_t resetCPU(CPU_t *);
CPU_Error_Code_t stepCPU(CPU_t *, memory_t *);

#endif
