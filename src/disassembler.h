/**
 * 65(c)816 emulator/simulator
 * (C) Ray Clemens 2023
 */

#ifndef _DISASSEMBLER_H
#define _DISASSEMBLER_H

#include "65816.h"

// Keep in sync with the instruction_mne array
typedef enum instruction_t {
    I_ADC = 0,
    I_AND,
    I_ASL,
    I_BCC,
    I_BCS,
    I_BEQ,
    I_BIT,
    I_BMI,
    I_BNE,
    I_BPL,
    I_BRA,
    I_BRK,
    I_BRL,
    I_BVC,
    I_BVS,
    I_CLC,
    I_CLD,
    I_CLI,
    I_CLV,
    I_CMP,
    I_COP,
    I_CPX,
    I_CPY,
    I_DEC,
    I_DEX,
    I_DEY,
    I_EOR,
    I_INC,
    I_INX,
    I_INY,
    I_JMP,
    I_JSL,
    I_JSR,
    I_LDA,
    I_LDX,
    I_LDY,
    I_LSR,
    I_MVN,
    I_MVP,
    I_NOP,
    I_ORA,
    I_PEA,
    I_PEI,
    I_PER,
    I_PHA,
    I_PHB,
    I_PHD,
    I_PHK,
    I_PHP,
    I_PHX,
    I_PHY,
    I_PLA,
    I_PLB,
    I_PLD,
    I_PLP,
    I_PLX,
    I_PLY,
    I_REP,
    I_ROL,
    I_ROR,
    I_RTI,
    I_RTL,
    I_RTS,
    I_SBC,
    I_SEC,
    I_SED,
    I_SEI,
    I_SEP,
    I_STA,
    I_STP,
    I_STX,
    I_STY,
    I_STZ,
    I_TAX,
    I_TAY,
    I_TCD,
    I_TCS,
    I_TDC,
    I_TRB,
    I_TSB,
    I_TSC,
    I_TSX,
    I_TXA,
    I_TXS,
    I_TXY,
    I_TYA,
    I_TYX,
    I_WAI,
    I_WDM,
    I_XBA,
    I_XCE
} instruction_t;
    
// The type of register that an instruction uses
// (Specifically, the register that the width of the operation is based on)
typedef enum regtype_t {
    REG__, // Generic
    REG_X,
    REG_A
} regtype_t;

// Opcode
typedef struct opcode_t {
    CPU_Addr_Mode_t addr_mode;
    instruction_t inst;
    regtype_t reg;
} opcode_t;


extern char instruction_mne[][4];
extern char addr_fmts[][16];
extern int addr_fmt_sizes[];
extern opcode_t opcode_table[];

#endif


