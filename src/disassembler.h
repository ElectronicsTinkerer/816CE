/**
 * 65(c)816 emulator/simulator
 * (C) Ray Clemens 2023
 */

#ifndef _DISASSEMBLER_H
#define _DISASSEMBLER_H

#include "65816.h"

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

char insturction_mne[][4] = {
    "ADC",
    "AND",
    "ASL",
    "BCC",
    "BCS",
    "BEQ",
    "BIT",
    "BMI",
    "BNE",
    "BPL",
    "BRA",
    "BRK",
    "BVC",
    "BVS",
    "CLC",
    "CLD",
    "CLI",
    "CLV",
    "CMP",
    "COP",
    "CPX",
    "CPY",
    "DEC",
    "DEX",
    "DEY",
    "EOR",
    "INC",
    "INX",
    "INY",
    "JMP",
    "JSL",
    "JSR",
    "LDA",
    "LDX",
    "LDY",
    "LSR",
    "MVN",
    "MVP",
    "NOP",
    "ORA",
    "PEA",
    "PEI",
    "PER",
    "PHA",
    "PHB",
    "PHD",
    "PHK",
    "PHP",
    "PHX",
    "PHY",
    "PLA",
    "PLB",
    "PLD",
    "PLP",
    "PLX",
    "PLY",
    "REP",
    "ROL",
    "ROR",
    "RTI",
    "RTL",
    "RTS",
    "SBC",
    "SEC",
    "SED",
    "SEI",
    "SEP",
    "STA",
    "STP",
    "STX",
    "STY",
    "STZ",
    "TAX",
    "TAY",
    "TCD",
    "TCS",
    "TDC",
    "TRB",
    "TSB",
    "TSC",
    "TSX",
    "TXA",
    "TXS",
    "TXY",
    "TYA",
    "TYX",
    "WAI",
    "WDM",
    "XBA",
    "XCE"
};

// Addressing format strings
// (KEEP IN SYNC WITH CPU_Addr_Mode_t in 65816.h)
char addr_fmts[][16] = {
    "%02x",     // Direct page
    "%02x,X",   // Direct page indexed, x
    "(%02x,X)", // Direct page indexed indirect X
    "%02,Y",    // Direct page indexed, y
    "(%02x),Y", // Direct page indirect indexed y
    "[%02x],Y", // Direct page indirect long indexed y
    "(%02x)",   // Direct page indirect
    "[%02x]",   // Direct page indirect long
    "%04x",     // Absolute
    "%04x,X",   // Absolute indexed X
    "%04x,Y",   // Absolute indexed Y
    "(%04x)",   // Indirect absolute
    "%06x",     // Absolute long (24-bit)
    "%06x,X",   // Absolute long indexed x
    "(%04x,X)", // Absolute indirect indexed x
    "#%02",     // Immediate (MODIFIED DURING PRINTING)
    "%02x,S",   // Stack relative
    "(%02x,S),Y", // Stack relative indirect indexed y
    "",         // Implied
    "%02x,$02x",// Block move
    "%06x",     // Program counter relative (8-bit)
    "%06x"      // Program counter relative (16-bit)
};

// The number of bytes for each format
// (KEEP IN SYNC WITH CPU_Addr_Mode_t in 65816.h)
int addr_fmt_sizes[] = {
    2, // Direct page
    2, // Direct page indexed, x
    2, // Direct page indexed indirect X
    2, // Direct page indexed, y
    2, // Direct page indirect indexed y
    2, // Direct page indirect long indexed y
    2, // Direct page indirect
    2, // Direct page indirect long
    3, // Absolute
    3, // Absolute indexed X
    3, // Absolute indexed Y
    3, // Indirect absolute
    4, // Absolute long (24-bit)
    4, // Absolute long indexed x
    3, // Absolute indirect indexed x
    2, // Immediate (MODIFIED DURING PRINTING)
    2, // Stack relative
    2, // Stack relative indirect indexed y
    1, // Implied
    3, // Block move
    2, // Program counter relative (8-bit)
    3  // Program counter relative (16-bit)
};
    
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

// Opcode table
opcode_t opcode_table[] = {
    {CPU_ADDR_DP,      I_BRK, REG__}, // 0
    {CPU_ADDR_DPINDX,  I_ORA, REG_A},
    {CPU_ADDR_DP,      I_COP, REG__},
    {CPU_ADDR_SR,      I_ORA, REG_A},
    {CPU_ADDR_DP,      I_TSB, REG_A},
    {CPU_ADDR_DP,      I_ORA, REG_A},
    {CPU_ADDR_DP,      I_ASL, REG_A},
    {CPU_ADDR_DPINDL,  I_ORA, REG_A},
    {CPU_ADDR_IMPD,    I_PHP, REG__},
    {CPU_ADDR_IMMD,    I_ORA, REG_A},
    {CPU_ADDR_IMPD,    I_ASL, REG_A},
    {CPU_ADDR_IMPD,    I_PHD, REG__},
    {CPU_ADDR_ABS,     I_TSB, REG_A},
    {CPU_ADDR_ABS,     I_ORA, REG_A},
    {CPU_ADDR_ABS,     I_ASL, REG_A},
    {CPU_ADDR_ABSL,    I_ORA, REG_A},
    {CPU_ADDR_BPL,     I_BPL, REG__}, // 10
    {CPU_ADDR_INDDPY,  I_ORA, REG_A},
    {CPU_ADDR_DPIND,   I_ORA, REG_A},
    {CPU_ADDR_SRINDY,  I_ORA, REG_A},
    {CPU_ADDR_DP,      I_TRB, REG_A},
    {CPU_ADDR_DPX,     I_ORA, REG_A},
    {CPU_ADDR_DPX,     I_ASL, REG_A},
    {CPU_ADDR_INDDPLY, I_ORA, REG_A},
    {CPU_ADDR_IMPD,    I_CLC, REG__},
    {CPU_ADDR_ABSY,    I_ORA, REG_A},
    {CPU_ADDR_IMPD,    I_INC, REG_A},
    {CPU_ADDR_IMPD,    I_TCS, REG_A},
    {CPU_ADDR_ABS,     I_TRB, REG_A},
    {CPU_ADDR_ABSX,    I_ORA, REG_A},
    {CPU_ADDR_ABSX,    I_ASL, REG_A},
    {CPU_ADDR_ABSLX,   I_ORA, REG_A},
    {CPU_ADDR_ABS,     I_JSR, REG__}, // 20
    {CPU_ADDR_DPINDX,  I_AND, REG_A},
    {CPU_ADDR_ABSL,    I_JSR, REG__},
    {CPU_ADDR_SR,      I_AND, REG_A},
    {CPU_ADDR_DP,      I_BIT, REG_A},
    {CPU_ADDR_DP,      I_AND, REG_A},
    {CPU_ADDR_DP,      I_ROL, REG_A},
    {CPU_ADDR_DPINDL,  I_AND, REG_A},
    {CPU_ADDR_IMPD,    I_PLP, REG__},
    {CPU_ADDR_IMMD,    I_AND, REG_A},
    {CPU_ADDR_IMPD,    I_ROL, REG_A},
    {CPU_ADDR_IMPD,    I_PLD, REG__},
    {CPU_ADDR_ABS,     I_BIT, REG_A},
    {CPU_ADDR_ABS,     I_AND, REG_A},
    {CPU_ADDR_ABS,     I_ROL, REG_A},
    {CPU_ADDR_ABSL,    I_AND, REG_A},
    {CPU_ADDR_DP,      I_BMI, REG__}, // 30
    {CPU_ADDR_INDDPY,  I_AND, REG_A},
    {CPU_ADDR_DPIND,   I_AND, REG_A},
    {CPU_ADDR_SRINDY,  I_AND, REG_A},
    {CPU_ADDR_DPINDX,  I_BIT, REG_A},
    {CPU_ADDR_DPINDX,  I_AND, REG_A},
    {CPU_ADDR_DPINDX,  I_ROL, REG_A},
    {CPU_ADDR_INDDPLY, I_AND, REG_A},
    {CPU_ADDR_IMPD,    I_SEC, REG__},
    {CPU_ADDR_ABSY,    I_AND, REG_A},
    {CPU_ADDR_IMPD,    I_DEC, REG_A},
    {CPU_ADDR_IMPD,    I_TSC, REG_A},
    {CPU_ADDR_ABSX,    I_BIT, REG_A},
    {CPU_ADDR_ABSX,    I_AND, REG_A},
    {CPU_ADDR_ABSX,    I_ROL, REG_A},
    {CPU_ADDR_ABSLX,   I_AND, REG_A},
    {CPU_ADDR_IMPD,    I_RTI, REG__}, // 40
    {CPU_ADDR_DPINDX,  I_EOR, REG_A},
    {CPU_ADDR_IMMD,    I_WDM, REG__},
    {CPU_ADDR_SR,      I_EOR, REG_A},
    {CPU_ADDR_BMV,     I_MVP, REG__},
    {CPU_ADDR_DP,      I_EOR, REG_A},
    {CPU_ADDR_DP,      I_LSR, REG_A},
    {CPU_ADDR_DPINDL,  I_EOR, REG_A},
    {CPU_ADDR_IMPD,    I_PHA, REG_A},
    {CPU_ADDR_IMMD,    I_EOR, REG_A},
    {CPU_ADDR_IMPD,    I_LSR, REG_A},
    {CPU_ADDR_IMPD,    I_PHK, REG__},
    {CPU_ADDR_ABS,     I_JMP, REG__},
    {CPU_ADDR_ABS,     I_EOR, REG_A},
    {CPU_ADDR_ABS,     I_LSR, REG_A},
    {CPU_ADDR_ABSL,    I_EOR, REG_A},
    {CPU_ADDR_PCR,     I_BVC, REG__}, // 50
    {CPU_ADDR_INDDPY,  I_EOR, REG_A},
    {CPU_ADDR_DPIND,   I_EOR, REG_A},
    {CPU_ADDR_SRINDY,  I_EOR, REG_A},
    {CPU_ADDR_BMV,     I_MVN, REG__},
    {CPU_ADDR_DPX,     I_EOR, REG_A},
    {CPU_ADDR_DPX,     I_LSR, REG_A},
    {CPU_ADDR_INDDPLY, I_EOR, REG_A},
    {CPU_ADDR_IMPD,    I_CLI, REG__},
    {CPU_ADDR_ABSY,    I_EOR, REG_A},
    {CPU_ADDR_IMPD,    I_PHY, REG_X},
    {CPU_ADDR_IMPD,    I_TCD, REG_A},
    {CPU_ADDR_ABSL,    I_JMP, REG_A},
    {CPU_ADDR_ABSX,    I_EOR, REG_A},
    {CPU_ADDR_ABSX,    I_LSR, REG_A},
    {CPU_ADDR_ABSLX,   I_EOR, REG_A},
    {CPU_ADDR_IMPD,    I_RTS, REG__}, // 60
    {CPU_ADDR_DPINDX,  I_ADC, REG_A},
    {CPU_ADDR_ABS,     I_PER, REG__}, // PC-relative long is displayed as absolute
    {CPU_ADDR_SR,      I_ADC, REG_A},
    {CPU_ADDR_DP,      I_STZ, REG_A},
    {CPU_ADDR_DP,      I_ADC, REG_A},
    {CPU_ADDR_DP,      I_ROR, REG_A},
    {CPU_ADDR_DPINDL,  I_ADC, REG_A},
    {CPU_ADDR_IMPD,    I_PLA, REG_A},
    {CPU_ADDR_IMMD,    I_ADC, REG_A},
    {CPU_ADDR_IMPD,    I_ROR, REG_A},
    {CPU_ADDR_IMPD,    I_RTL, REG__},
    {CPU_ADDR_ABSIND,  I_JMP, REG__},
    {CPU_ADDR_ABS,     I_ADC, REG_A},
    {CPU_ADDR_ABS,     I_ROR, REG_A},
    {CPU_ADDR_ABSL,    I_ADC, REG_A},
    {CPU_ADDR_PCR,     I_BVS, REG__}, // 70
    {CPU_ADDR_INDDPY,  I_ADC, REG_A},
    {CPU_ADDR_DPIND,   I_ADC, REG_A},
    {CPU_ADDR_SRINDY,  I_ADC, REG_A},
    {CPU_ADDR_DPX,     I_STZ, REG_A},
    {CPU_ADDR_DPX,     I_ADC, REG_A},
    {CPU_ADDR_DPX,     I_ROR, REG_A},
    {CPU_ADDR_INDDPLY, I_ADC, REG_A},
    {CPU_ADDR_IMPD,    I_SEI, REG__},
    {CPU_ADDR_ABSY,    I_ADC, REG_A},
    {CPU_ADDR_IMPD,    I_PLY, REG_X},
    {CPU_ADDR_IMPD,    I_TDC, REG_A},
    {CPU_ADDR_ABSINDX, I_JMP, REG__},
    {CPU_ADDR_ABSX,    I_ADC, REG_A},
    {CPU_ADDR_ABSX,    I_ROR, REG_A},
    {CPU_ADDR_ABSLX,   I_ADC, REG_A},
    {CPU_ADDR_PCR,     I_BRA, REG__}, // 80
    {CPU_ADDR_DPINDX,  I_STA, REG_A},
    {CPU_ADDR_PCRL,    I_BRL, REG__},
    {CPU_ADDR_SR,      I_STA, REG_A},
    {CPU_ADDR_DP,      I_STY, REG_X},
    {CPU_ADDR_DP,      I_STA, REG_A},
    {CPU_ADDR_DP,      I_STX, REG_X},
    {CPU_ADDR_DPINDL,  I_STA, REG_A},
    {CPU_ADDR_IMPD,    I_DEY, REG_X},
    {CPU_ADDR_IMMD,    I_BIT, REG_A},
    {CPU_ADDR_IMPD,    I_TXA, REG_A}, // Could be REG_X
    {CPU_ADDR_IMPD,    I_PHB, REG__},
    {CPU_ADDR_ABS,     I_STY, REG_X},
    {CPU_ADDR_ABS,     I_STA, REG_A},
    {CPU_ADDR_ABS,     I_STX, REG_X},
    {CPU_ADDR_ABSL,    I_STA, REG_A},
    {CPU_ADDR_PCR,     I_BCC, REG__}, // 90
    {CPU_ADDR_INDDPY,  I_STA, REG_A},
    {CPU_ADDR_DPIND,   I_STA, REG_A},
    {CPU_ADDR_SRINDY,  I_STA, REG_A},
    {CPU_ADDR_DPX,     I_STY, REG_X},
    {CPU_ADDR_DPX,     I_STA, REG_A},
    {CPU_ADDR_DPY,     I_STX, REG_X},
    {CPU_ADDR_INDDPLY, I_STA, REG_A},
    {CPU_ADDR_IMPD,    I_TYA, REG_A}, // Could be REG_X
    {CPU_ADDR_ABSY,    I_STA, REG_X},
    {CPU_ADDR_IMPD,    I_TXS, REG_X},
    {CPU_ADDR_IMPD,    I_TXY, REG_X},
    {CPU_ADDR_ABS,     I_STZ, REG_A},
    {CPU_ADDR_ABSX,    I_STA, REG_A},
    {CPU_ADDR_ABSX,    I_STZ, REG_A},
    {CPU_ADDR_ABSLX,   I_STA, REG_A},
    {CPU_ADDR_IMMD,    I_LDY, REG_X}, // A0
    {CPU_ADDR_DPINDX,  I_LDA, REG_A},
    {CPU_ADDR_IMMD,    I_LDX, REG_X},
    {CPU_ADDR_SR,      I_LDA, REG_A},
    {CPU_ADDR_DP,      I_LDY, REG_X},
    {CPU_ADDR_DP,      I_LDA, REG_A},
    {CPU_ADDR_DP,      I_LDX, REG_X},
    {CPU_ADDR_DPINDL,  I_LDA, REG_A},
    {CPU_ADDR_IMPD,    I_TAY, REG_X}, // Could be REG_A
    {CPU_ADDR_IMMD,    I_LDA, REG_A},
    {CPU_ADDR_IMPD,    I_TAX, REG_X}, // Could be REG_A
    {CPU_ADDR_IMPD,    I_PLB, REG__},
    {CPU_ADDR_ABS,     I_LDY, REG_X},
    {CPU_ADDR_ABS,     I_LDA, REG_A},
    {CPU_ADDR_ABS,     I_LDX, REG_X},
    {CPU_ADDR_ABSL,    I_LDA, REG_A},
    {CPU_ADDR_PCR,     I_BCS, REG__}, // B0
    {CPU_ADDR_INDDPY,  I_LDA, REG_A},
    {CPU_ADDR_DPIND,   I_LDA, REG_A},
    {CPU_ADDR_SRINDY,  I_LDA, REG_A},
    {CPU_ADDR_DPX,     I_LDY, REG_X},
    {CPU_ADDR_DPX,     I_LDA, REG_A},
    {CPU_ADDR_DPY,     I_LDX, REG_X},
    {CPU_ADDR_INDDPLY, I_LDA, REG_A},
    {CPU_ADDR_IMPD,    I_CLV, REG__},
    {CPU_ADDR_ABSY,    I_LDA, REG_A},
    {CPU_ADDR_IMPD,    I_TSX, REG_X},
    {CPU_ADDR_IMPD,    I_TYX, REG_X},
    {CPU_ADDR_ABSX,    I_LDY, REG_X},
    {CPU_ADDR_ABSX,    I_LDA, REG_A},
    {CPU_ADDR_ABSY,    I_LDX, REG_X},
    {CPU_ADDR_ABSLX,   I_LDA, REG_A},
    {CPU_ADDR_IMMD,    I_CPY, REG_X}, // C0
    {CPU_ADDR_DPINDX,  I_CMP, REG_A},
    {CPU_ADDR_IMMD,    I_REP, REG__},
    {CPU_ADDR_SR,      I_CMP, REG_A},
    {CPU_ADDR_DP,      I_CPY, REG_X},
    {CPU_ADDR_DP,      I_CMP, REG_A},
    {CPU_ADDR_DP,      I_DEC, REG_A},
    {CPU_ADDR_DPINDL,  I_CMP, REG_A},
    {CPU_ADDR_IMPD,    I_INY, REG_X},
    {CPU_ADDR_IMMD,    I_CMP, REG_A},
    {CPU_ADDR_IMPD,    I_DEX, REG_X},
    {CPU_ADDR_IMPD,    I_WAI, REG__},
    {CPU_ADDR_ABS,     I_CPY, REG_X},
    {CPU_ADDR_ABS,     I_CMP, REG_A},
    {CPU_ADDR_ABS,     I_DEC, REG_A},
    {CPU_ADDR_ABSL,    I_CMP, REG_A},
    {CPU_ADDR_PCR,     I_BNE, REG__}, // D0
    {CPU_ADDR_INDDPY,  I_CMP, REG_A},
    {CPU_ADDR_DPIND,   I_CMP, REG_A},
    {CPU_ADDR_SRINDY,  I_CMP, REG_A},
    {CPU_ADDR_DPIND,   I_PEI, REG__},
    {CPU_ADDR_DPX,     I_CMP, REG_A},
    {CPU_ADDR_DPX,     I_DEC, REG_A},
    {CPU_ADDR_INDDPLY, I_CMP, REG_A},
    {CPU_ADDR_IMPD,    I_CLD, REG__},
    {CPU_ADDR_ABSY,    I_CMP, REG_A},
    {CPU_ADDR_IMPD,    I_PHX, REG_X},
    {CPU_ADDR_IMPD,    I_STP, REG__},
    {CPU_ADDR_ABSINDL, I_JMP, REG__},
    {CPU_ADDR_ABSX,    I_CMP, REG_A},
    {CPU_ADDR_ABSX,    I_DEC, REG_A},
    {CPU_ADDR_ABSLX,   I_CMP, REG_A},
    {CPU_ADDR_IMMD,    I_CPX, REG_X}, // E0
    {CPU_ADDR_DPINDX,  I_SBC, REG_A},
    {CPU_ADDR_IMMD,    I_SEP, REG__},
    {CPU_ADDR_SR,      I_SBC, REG_A},
    {CPU_ADDR_DP,      I_CPX, REG_X},
    {CPU_ADDR_DP,      I_SBC, REG_A},
    {CPU_ADDR_DP,      I_INC, REG_A},
    {CPU_ADDR_DPINDL,  I_SBC, REG_A},
    {CPU_ADDR_IMPD,    I_INX, REG_X},
    {CPU_ADDR_IMMD,    I_SBC, REG_A},
    {CPU_ADDR_IMPD,    I_NOP, REG__},
    {CPU_ADDR_IMPD,    I_XBA, REG__},
    {CPU_ADDR_ABS,     I_CPX, REG_X},
    {CPU_ADDR_ABS,     I_SBC, REG_A},
    {CPU_ADDR_ABS,     I_INC, REG_A},
    {CPU_ADDR_ABSL,    I_SBC, REG_A},
    {CPU_ADDR_PCR,     I_BEQ, REG__}, // F0
    {CPU_ADDR_INDDPY,  I_SBC, REG_A},
    {CPU_ADDR_DPIND,   I_SBC, REG_A},
    {CPU_ADDR_SRINDY,  I_SBC, REG_A},
    {CPU_ADDR_ABS,     I_PEA, REG__},
    {CPU_ADDR_DPX,     I_SBC, REG_A},
    {CPU_ADDR_DPX,     I_INC, REG_A},
    {CPU_ADDR_INDDPLY, I_SBC, REG_A},
    {CPU_ADDR_IMPD,    I_SED, REG__},
    {CPU_ADDR_ABSY,    I_SBC, REG_A},
    {CPU_ADDR_IMPD,    I_PLX, REG_X},
    {CPU_ADDR_IMPD,    I_XCE, REG__},
    {CPU_ADDR_ABSINDX, I_JSR, REG__},
    {CPU_ADDR_ABSX,    I_SBC, REG_A},
    {CPU_ADDR_ABSX,    I_INC, REG_A},
    {CPU_ADDR_ABSLX,   I_SBC, REG_A}
     // TODO: FIX PER? ADDRESSING MODE TO BE RELATIVE?
};
    

#endif


