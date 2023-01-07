
#include "65816.h"
#include "65816-ops.h"
#include "65816-util.h"

/**
 * Resets a CPU to its post-/RST value
 * @param cpu The CPU to be reset
 */
CPU_Error_Code_t resetCPU(CPU_t *cpu)
{
#ifdef CPU_DEBUG_CHECK_NULL
    if (cpu == NULL)
    {
        return CPU_ERR_NULL_CPU;
    }
#endif

    cpu->D = 0x0000;
    cpu->DBR = 0x00;
    cpu->PBR = 0x00;
    cpu->SP &= 0x00ff; // Only keep low byte of SP
    cpu->SP |= 0x0100;
    cpu->X &= 0x00ff; // Only keep low byte of X
    cpu->Y &= 0x00ff; // Only keep low byte of Y
    cpu->P.M = 1;
    cpu->P.XB = 1;
    cpu->P.D = 0;
    cpu->P.I = 1;
    cpu->P.E = 1;

    // SIM extra state vars
    cpu->cycles = 0;
    cpu->P.CRASH = 0;

    // Internal use only, tell the sim that the CPU just reset
    cpu->P.RST = 1;

    return CPU_ERR_OK;
}

/**
 * Steps a CPU by one machine cycle
 * @param cpu The CPU to be stepped
 * @param mem The memory array which is to be connected to the CPU
 */
CPU_Error_Code_t stepCPU(CPU_t *cpu, memory_t *mem)
{
#ifdef CPU_DEBUG_CHECK_NULL
    if (cpu == NULL)
    {
        return CPU_ERR_NULL_CPU;
    }
#endif

    if (cpu->P.CRASH == 1)
    {
        return CPU_ERR_CRASH;
    }

    // Handle CPU reset (does not perform a full CPU reset)
    if (cpu->P.RST)
    {
        cpu->PC = _get_mem_word(mem, CPU_VEC_RESET);
        return CPU_ERR_OK;
    }

    if (cpu->P.STP)
    {
        return CPU_ERR_STP;
    }

    // Fetch, decode, execute instruction
    switch (_get_mem_byte(mem, cpu->PC))
    {
    case 0x00: i_brk(cpu, mem); break;
    case 0x01: i_ora(cpu, mem, 2, 6, CPU_ADDR_DPINDX, _addrCPU_getDirectPageIndexedIndirectX(cpu, mem)); break;
    case 0x02: i_cop(cpu, mem); break;
    case 0x03: i_ora(cpu, mem, 2, 4, CPU_ADDR_SR, _addrCPU_getStackRelative(cpu, mem)); break;

    case 0x05: i_ora(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
    case 0x06: i_asl(cpu, mem, 2, 5, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
    case 0x07: i_ora(cpu, mem, 2, 6, CPU_ADDR_DPINDL, _addrCPU_getDirectPageIndirectLong(cpu, mem)); break;
    case 0x08: i_php(cpu, mem); break;
    case 0x09: i_ora(cpu, mem, 2, 2, CPU_ADDR_IMMD, _addrCPU_getImmediate(cpu, mem)); break;
    case 0x0a: i_asl(cpu, mem, 1, 2, CPU_ADDR_IMPD, 0); break;
    case 0x0b: i_phd(cpu, mem); break;

    case 0x0d: i_ora(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
    case 0x0e: i_asl(cpu, mem, 3, 6, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
    case 0x0f: i_ora(cpu, mem, 4, 5, CPU_ADDR_ABSL, _addrCPU_getLong(cpu, mem)); break;
    case 0x10: i_bpl(cpu, mem); break;
    case 0x11: i_ora(cpu, mem, 2, 5, CPU_ADDR_INDDPY, _addrCPU_getDirectPageIndirectIndexedY(cpu, mem)); break;
    case 0x12: i_ora(cpu, mem, 2, 5, CPU_ADDR_DPIND, _addrCPU_getDirectPageIndirect(cpu, mem)); break;
    case 0x13: i_ora(cpu, mem, 2, 7, CPU_ADDR_SRINDY, _addrCPU_getStackRelativeIndirectIndexedY(cpu, mem)); break;

    case 0x15: i_ora(cpu, mem, 2, 4, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
    case 0x16: i_asl(cpu, mem, 2, 6, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
    case 0x17: i_ora(cpu, mem, 2, 6, CPU_ADDR_INDDPLY, _addrCPU_getDirectPageIndirectLongIndexedY(cpu, mem)); break;
    case 0x18: i_clc(cpu); break;
    case 0x19: i_ora(cpu, mem, 3, 4, CPU_ADDR_ABSY, _addrCPU_getAbsoluteIndexedY(cpu, mem)); break;
    case 0x1a: i_ina(cpu); break;
    case 0x1b: i_tcs(cpu); break;

    case 0x1d: i_ora(cpu, mem, 3, 4, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
    case 0x1e: i_asl(cpu, mem, 3, 7, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
    case 0x1f: i_ora(cpu, mem, 4, 5, CPU_ADDR_ABSLX, _addrCPU_getLongIndexedX(cpu, mem)); break;
    case 0x20: i_jsr(cpu, mem, 6, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
    case 0x21: i_and(cpu, mem, 2, 6, CPU_ADDR_DPINDX, _addrCPU_getDirectPageIndexedIndirectX(cpu, mem)); break;
    case 0x22: i_jsl(cpu, mem, 6, CPU_ADDR_ABS, _addrCPU_getLong(cpu, mem)); break;
    case 0x23: i_and(cpu, mem, 2, 4, CPU_ADDR_SR, _addrCPU_getStackRelative(cpu, mem)); break;
    case 0x24: i_bit(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0x25: i_and(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0x26: i_rol(cpu, mem, 2, 5, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0x27: i_and(cpu, mem, 2, 6, CPU_ADDR_DPINDL, _addrCPU_getDirectPageIndirectLong(cpu, mem)); break;
        case 0x28: i_plp(cpu, mem); break;
        case 0x29: i_and(cpu, mem, 2, 2, CPU_ADDR_IMMD, _addrCPU_getImmediate(cpu, mem)); break;
        case 0x2a: i_rol(cpu, mem, 1, 2, CPU_ADDR_IMPD, 0); break;
        case 0x2b: i_pld(cpu, mem); break;
        case 0x2c: i_bit(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0x2d: i_and(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0x2e: i_rol(cpu, mem, 3, 6, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0x2f: i_and(cpu, mem, 4, 5, CPU_ADDR_ABSL, _addrCPU_getLong(cpu, mem)); break;
        case 0x30: i_bmi(cpu, mem); break;
        case 0x31: i_and(cpu, mem, 2, 5, CPU_ADDR_INDDPY, _addrCPU_getDirectPageIndirectIndexedY(cpu, mem)); break;
        case 0x32: i_and(cpu, mem, 2, 5, CPU_ADDR_DPIND, _addrCPU_getDirectPageIndirect(cpu, mem)); break;
        case 0x33: i_and(cpu, mem, 2, 7, CPU_ADDR_SRINDY, _addrCPU_getStackRelativeIndirectIndexedY(cpu, mem)); break;
        case 0x34: i_bit(cpu, mem, 2, 4, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0x35: i_and(cpu, mem, 2, 4, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0x36: i_rol(cpu, mem, 2, 6, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0x37: i_and(cpu, mem, 2, 6, CPU_ADDR_INDDPLY, _addrCPU_getDirectPageIndirectLongIndexedY(cpu, mem)); break;
        case 0x38: i_sec(cpu); break;
        case 0x39: i_and(cpu, mem, 3, 4, CPU_ADDR_ABSY, _addrCPU_getAbsoluteIndexedY(cpu, mem)); break;
        case 0x3a: i_dea(cpu); break;
        case 0x3b: i_tsc(cpu); break;
        case 0x3c: i_bit(cpu, mem, 3, 4, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0x3d: i_and(cpu, mem, 3, 4, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0x3e: i_rol(cpu, mem, 3, 7, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0x3f: i_and(cpu, mem, 4, 5, CPU_ADDR_ABSLX, _addrCPU_getLongIndexedX(cpu, mem)); break;
        case 0x40: i_rti(cpu, mem); break;
        case 0x41: i_eor(cpu, mem, 2, 6, CPU_ADDR_DPINDX, _addrCPU_getDirectPageIndexedIndirectX(cpu, mem)); break;
        case 0x42: i_wdm(cpu); break;
        case 0x43: i_eor(cpu, mem, 2, 4, CPU_ADDR_SR, _addrCPU_getStackRelative(cpu, mem)); break;
        case 0x44: i_mvp(cpu, mem);
        case 0x45: i_eor(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0x46: i_lsr(cpu, mem, 2, 5, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0x47: i_eor(cpu, mem, 2, 6, CPU_ADDR_DPINDL, _addrCPU_getDirectPageIndirectLong(cpu, mem)); break;
        case 0x48: i_pha(cpu, mem); break;
        case 0x49: i_eor(cpu, mem, 2, 2, CPU_ADDR_IMMD, _addrCPU_getImmediate(cpu, mem)); break;
        case 0x4a: i_lsr(cpu, mem, 1, 2, CPU_ADDR_IMPD, 0); break;
        case 0x4b: i_phk(cpu, mem); break;
        case 0x4c: i_jmp(cpu, mem, 3, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0x4d: i_eor(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0x4e: i_lsr(cpu, mem, 3, 6, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0x4f: i_eor(cpu, mem, 4, 5, CPU_ADDR_ABSL, _addrCPU_getLong(cpu, mem)); break;
        case 0x50: i_bvc(cpu, mem); break;
        case 0x51: i_eor(cpu, mem, 2, 5, CPU_ADDR_INDDPY, _addrCPU_getDirectPageIndexedY(cpu, mem)); break;
        case 0x52: i_eor(cpu, mem, 2, 5, CPU_ADDR_DPIND, _addrCPU_getDirectPageIndirect(cpu, mem)); break;
        case 0x53: i_eor(cpu, mem, 2, 7, CPU_ADDR_SRINDY, _addrCPU_getStackRelativeIndirectIndexedY(cpu, mem)); break;
        case 0x54: i_mvn(cpu, mem);
        case 0x55: i_eor(cpu, mem, 2, 4, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0x56: i_lsr(cpu, mem, 2, 5, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0x57: i_eor(cpu, mem, 2, 6, CPU_ADDR_INDDPLY, _addrCPU_getDirectPageIndirectLongIndexedY(cpu, mem)); break;
        case 0x58: i_cli(cpu); break;
        case 0x59: i_eor(cpu, mem, 3, 4, CPU_ADDR_ABSY, _addrCPU_getAbsoluteIndexedY(cpu, mem)); break;
        case 0x5a: i_phy(cpu, mem); break;
        case 0x5b: i_tcd(cpu); break;
        case 0x5c: i_jmp(cpu, mem, 4, CPU_ADDR_ABSL, _addrCPU_getLong(cpu, mem)); break;
        case 0x5d: i_eor(cpu, mem, 3, 4, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0x5e: i_lsr(cpu, mem, 3, 7, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0x5f: i_eor(cpu, mem, 4, 5, CPU_ADDR_ABSLX, _addrCPU_getLongIndexedX(cpu, mem)); break;
        case 0x60: i_rts(cpu, mem); break;
        case 0x61: i_adc(cpu, mem, 2, 6, CPU_ADDR_DPINDX, _addrCPU_getDirectPageIndexedIndirectX(cpu, mem)); break;
        case 0x62: i_per(cpu, mem); break;
        case 0x63: i_adc(cpu, mem, 2, 4, CPU_ADDR_SR, _addrCPU_getStackRelative(cpu, mem)); break;
        case 0x64: i_stz(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0x65: i_adc(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0x66: i_ror(cpu, mem, 2, 5, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0x67: i_adc(cpu, mem, 2, 6, CPU_ADDR_DPINDL, _addrCPU_getDirectPageIndirectLong(cpu, mem)); break;
        case 0x68: i_pla(cpu, mem); break;
        case 0x69: i_adc(cpu, mem, 2, 2, CPU_ADDR_IMMD, _addrCPU_getImmediate(cpu, mem)); break;
        case 0x6a: i_ror(cpu, mem, 1, 2, CPU_ADDR_IMPD, 0); break;
        case 0x6b: i_rtl(cpu, mem); break;
        case 0x6c: i_jmp(cpu, mem, 5, CPU_ADDR_INDABS, _addrCPU_getAbsoluteIndirect(cpu, mem)); break;
        case 0x6d: i_adc(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0x6e: i_ror(cpu, mem, 3, 6, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0x6f: i_adc(cpu, mem, 4, 5, CPU_ADDR_ABSL, _addrCPU_getLong(cpu, mem)); break;
        case 0x70: i_bvs(cpu, mem); break;
        case 0x71: i_adc(cpu, mem, 2, 5, CPU_ADDR_INDDPY, _addrCPU_getDirectPageIndirectIndexedY(cpu, mem)); break;
        case 0x72: i_adc(cpu, mem, 2, 5, CPU_ADDR_DPIND, _addrCPU_getDirectPageIndirect(cpu, mem)); break;
        case 0x73: i_adc(cpu, mem, 2, 7, CPU_ADDR_SRINDY, _addrCPU_getStackRelativeIndirectIndexedY(cpu, mem)); break;
        case 0x74: i_stz(cpu, mem, 2, 4, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0x75: i_adc(cpu, mem, 2, 4, CPU_ADDR_DPINDX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0x76: i_ror(cpu, mem, 3, 6, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0x77: i_adc(cpu, mem, 2, 6, CPU_ADDR_INDDPLY, _addrCPU_getDirectPageIndirectLongIndexedY(cpu, mem)); break;
        case 0x78: i_sei(cpu); break;
        case 0x79: i_adc(cpu, mem, 3, 4, CPU_ADDR_ABSY, _addrCPU_getAbsoluteIndexedY(cpu, mem)); break;
        case 0x7a: i_ply(cpu, mem); break;
        case 0x7b: i_tdc(cpu); break;
        case 0x7c: i_jmp(cpu, mem, 6, CPU_ADDR_ABSINDX, _addrCPU_getAbsoluteIndexedIndirectX(cpu, mem)); break;
        case 0x7d: i_adc(cpu, mem, 3, 4, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0x7e: i_ror(cpu, mem, 3, 7, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0x7f: i_adc(cpu, mem, 4, 5, CPU_ADDR_ABSLX, _addrCPU_getLongIndexedX(cpu, mem)); break;
        case 0x80: i_bra(cpu, mem); break;
        case 0x81: i_sta(cpu, mem, 2, 6, CPU_ADDR_DPINDX, _addrCPU_getDirectPageIndexedIndirectX(cpu, mem)); break;
        case 0x82: i_brl(cpu, mem); break;
        case 0x83: i_sta(cpu, mem, 2, 4, CPU_ADDR_SR, _addrCPU_getStackRelative(cpu, mem)); break;
        case 0x84: i_sty(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0x85: i_sta(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0x86: i_stx(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0x87: i_sta(cpu, mem, 2, 6, CPU_ADDR_DPINDL, _addrCPU_getDirectPageIndirectLong(cpu, mem)); break;
        case 0x88: i_dey(cpu); break;
        case 0x89: i_bit(cpu, mem, 2, 2, CPU_ADDR_IMMD, _addrCPU_getImmediate(cpu, mem)); break;
        case 0x8a: i_txa(cpu); break;
        case 0x8b: i_phb(cpu, mem); break;
        case 0x8c: i_sty(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0x8d: i_sta(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0x8e: i_stx(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0x8f: i_sta(cpu, mem, 4, 5, CPU_ADDR_ABSL, _addrCPU_getLong(cpu, mem)); break;
        case 0x90: i_bcc(cpu, mem); break;
        case 0x91: i_sta(cpu, mem, 2, 6, CPU_ADDR_INDDPY, _addrCPU_getDirectPageIndirectIndexedY(cpu, mem)); break;
        case 0x92: i_sta(cpu, mem, 2, 5, CPU_ADDR_DPIND, _addrCPU_getDirectPageIndexedY(cpu, mem)); break;
        case 0x93: i_sta(cpu, mem, 2, 7, CPU_ADDR_SRINDY, _addrCPU_getStackRelativeIndirectIndexedY(cpu, mem)); break;
        case 0x94: i_sty(cpu, mem, 2, 4, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0x95: i_sta(cpu, mem, 2, 4, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0x96: i_stx(cpu, mem, 2, 4, CPU_ADDR_DPY, _addrCPU_getDirectPageIndexedY(cpu, mem)); break;
        case 0x97: i_sta(cpu, mem, 2, 6, CPU_ADDR_INDDPLY, _addrCPU_getDirectPageIndirectLongIndexedY(cpu, mem)); break;
        case 0x98: i_tya(cpu); break;
        case 0x99: i_sta(cpu, mem, 3, 5, CPU_ADDR_ABSY, _addrCPU_getAbsoluteIndexedY(cpu, mem)); break;
        case 0x9a: i_txs(cpu); break;
        case 0x9b: i_txy(cpu); break;
        case 0x9c: i_stz(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0x9d: i_sta(cpu, mem, 3, 5, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0x9e: i_stz(cpu, mem, 3, 5, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0x9f: i_sta(cpu, mem, 4, 5, CPU_ADDR_ABSLX, _addrCPU_getLongIndexedX(cpu, mem)); break;
        case 0xa0: i_ldy(cpu, mem, 2, 2, CPU_ADDR_IMMD, _addrCPU_getImmediate(cpu, mem)); break;
        case 0xa1: i_lda(cpu, mem, 2, 6, CPU_ADDR_DPINDX, _addrCPU_getDirectPageIndexedIndirectX(cpu, mem)); break;
        case 0xa2: i_ldx(cpu, mem, 2, 2, CPU_ADDR_IMMD, _addrCPU_getImmediate(cpu, mem)); break;
        case 0xa3: i_lda(cpu, mem, 2, 4, CPU_ADDR_SR, _addrCPU_getStackRelative(cpu, mem)); break;
        case 0xa4: i_ldy(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0xa5: i_lda(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0xa6: i_ldx(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0xa7: i_lda(cpu, mem, 2, 6, CPU_ADDR_DPINDL, _addrCPU_getDirectPageIndirectLong(cpu, mem)); break;
        case 0xa8: i_tay(cpu); break;
        case 0xa9: i_lda(cpu, mem, 2, 2, CPU_ADDR_IMMD, _addrCPU_getImmediate(cpu, mem)); break;
        case 0xaa: i_tax(cpu); break;
        case 0xab: i_plb(cpu, mem); break;
        case 0xac: i_ldy(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0xad: i_lda(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0xae: i_ldx(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0xaf: i_lda(cpu, mem, 4, 5, CPU_ADDR_ABSL, _addrCPU_getLong(cpu, mem)); break;
        case 0xb0: i_bcs(cpu, mem); break;
        case 0xb1: i_lda(cpu, mem, 2, 5, CPU_ADDR_INDDPY, _addrCPU_getDirectPageIndirectIndexedY(cpu, mem)); break;
        case 0xb2: i_lda(cpu, mem, 2, 5, CPU_ADDR_DPIND, _addrCPU_getDirectPageIndirect(cpu, mem)); break;
        case 0xb3: i_lda(cpu, mem, 2, 7, CPU_ADDR_SRINDY, _addrCPU_getStackRelativeIndirectIndexedY(cpu, mem)); break;
        case 0xb4: i_ldy(cpu, mem, 2, 4, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0xb5: i_lda(cpu, mem, 2, 4, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0xb6: i_ldx(cpu, mem, 2, 4, CPU_ADDR_DPY, _addrCPU_getDirectPageIndexedY(cpu, mem)); break;
        case 0xb7: i_lda(cpu, mem, 2, 6, CPU_ADDR_INDDPLY, _addrCPU_getDirectPageIndirectLongIndexedY(cpu, mem)); break;
        case 0xb8: i_clv(cpu); break;
        case 0xb9: i_lda(cpu, mem, 3, 4, CPU_ADDR_ABSY, _addrCPU_getAbsoluteIndexedY(cpu, mem)); break;
        case 0xba: i_tsx(cpu); break;
        case 0xbb: i_tyx(cpu); break;
        case 0xbc: i_ldy(cpu, mem, 3, 4, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0xbd: i_lda(cpu, mem, 3, 4, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0xbe: i_ldx(cpu, mem, 3, 4, CPU_ADDR_ABSY, _addrCPU_getAbsoluteIndexedY(cpu, mem)); break;
        case 0xbf: i_lda(cpu, mem, 4, 5, CPU_ADDR_ABSLX, _addrCPU_getLongIndexedX(cpu, mem)); break;
        case 0xc0: i_cpy(cpu, mem, 2, 2, CPU_ADDR_IMMD, _addrCPU_getImmediate(cpu, mem)); break;
        case 0xc1: i_cmp(cpu, mem, 2, 6, CPU_ADDR_DPINDX, _addrCPU_getDirectPageIndexedIndirectX(cpu, mem)); break;
        case 0xc2: i_rep(cpu, mem); break;
        case 0xc3: i_cmp(cpu, mem, 2, 4, CPU_ADDR_SR, _addrCPU_getStackRelative(cpu, mem)); break;        case 0xc4: i_cpy(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0xc5: i_cmp(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0xc6: i_dec(cpu, mem, 2, 5, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0xc7: i_cmp(cpu, mem, 2, 6, CPU_ADDR_DPINDL, _addrCPU_getDirectPageIndirectLong(cpu, mem)); break;
        case 0xc8: i_iny(cpu); break;
        case 0xc9: i_cmp(cpu, mem, 2, 2, CPU_ADDR_IMMD, _addrCPU_getImmediate(cpu, mem)); break;
        case 0xca: i_dex(cpu); break;
        case 0xcb: i_wai(cpu); break;
        case 0xcc: i_cpy(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0xcd: i_cmp(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0xce: i_dec(cpu, mem, 3, 6, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0xcf: i_cmp(cpu, mem, 4, 5, CPU_ADDR_ABSL, _addrCPU_getLong(cpu, mem)); break;
        case 0xd0: i_bne(cpu, mem); break;
        case 0xd1: i_cmp(cpu, mem, 2, 5, CPU_ADDR_INDDPY, _addrCPU_getDirectPageIndirectIndexedY(cpu, mem)); break;
        case 0xd2: i_cmp(cpu, mem, 2, 5, CPU_ADDR_DPIND, _addrCPU_getDirectPageIndirect(cpu, mem)); break;
        case 0xd3: i_cmp(cpu, mem, 2, 7, CPU_ADDR_SRINDY, _addrCPU_getStackRelativeIndirectIndexedY(cpu, mem)); break;
        case 0xd4: i_pei(cpu, mem); break;
        case 0xd5: i_cmp(cpu, mem, 2, 4, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0xd6: i_dec(cpu, mem, 2, 6, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0xd7: i_cmp(cpu, mem, 2, 6, CPU_ADDR_INDDPLY, _addrCPU_getDirectPageIndirectLongIndexedY(cpu, mem)); break;
        case 0xd8: i_cld(cpu); break;
        case 0xd9: i_cmp(cpu, mem, 3, 4, CPU_ADDR_ABSY, _addrCPU_getAbsoluteIndexedY(cpu, mem)); break;
        case 0xda: i_phx(cpu, mem); break;
        case 0xdb: i_stp(cpu); break;
        case 0xdc: i_jmp(cpu, mem, 6, CPU_ADDR_ABSINDL, _addrCPU_getAbsoluteIndirectLong(cpu, mem)); break;
        case 0xdd: i_cmp(cpu, mem, 3, 4, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0xde: i_dec(cpu, mem, 3, 7, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0xdf: i_cmp(cpu, mem, 4, 5, CPU_ADDR_ABSLX, _addrCPU_getLongIndexedX(cpu, mem)); break;
        case 0xe0: i_cpx(cpu, mem, 2, 2, CPU_ADDR_IMMD, _addrCPU_getImmediate(cpu, mem)); break;
        case 0xe1: i_sbc(cpu, mem, 2, 6, CPU_ADDR_DPINDX, _addrCPU_getDirectPageIndexedIndirectX(cpu, mem)); break;
        case 0xe2: i_sep(cpu, mem); break;
        case 0xe3: i_sbc(cpu, mem, 2, 4, CPU_ADDR_SR, _addrCPU_getStackRelative(cpu, mem)); break;
        case 0xe4: i_cpx(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0xe5: i_sbc(cpu, mem, 2, 3, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0xe6: i_inc(cpu, mem, 2, 5, CPU_ADDR_DP, _addrCPU_getDirectPage(cpu, mem)); break;
        case 0xe7: i_sbc(cpu, mem, 2, 6, CPU_ADDR_DPINDL, _addrCPU_getDirectPageIndirectLong(cpu, mem)); break;
        case 0xe8: i_inx(cpu); break;
        case 0xe9: i_sbc(cpu, mem, 2, 2, CPU_ADDR_IMMD, _addrCPU_getImmediate(cpu, mem)); break;
        case 0xea: i_nop(cpu); break;
        case 0xeb: i_xba(cpu); break;
        case 0xec: i_cpx(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0xed: i_sbc(cpu, mem, 3, 4, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0xee: i_inc(cpu, mem, 3, 6, CPU_ADDR_ABS, _addrCPU_getAbsolute(cpu, mem)); break;
        case 0xef: i_sbc(cpu, mem, 4, 5, CPU_ADDR_ABSL, _addrCPU_getLong(cpu, mem)); break;
        case 0xf0: i_beq(cpu, mem); break;
        case 0xf1: i_sbc(cpu, mem, 2, 5, CPU_ADDR_INDDPY, _addrCPU_getDirectPageIndirectIndexedY(cpu, mem)); break;
        case 0xf2: i_sbc(cpu, mem, 2, 5, CPU_ADDR_DPIND, _addrCPU_getDirectPageIndirect(cpu, mem)); break;
        case 0xf3: i_sbc(cpu, mem, 2, 7, CPU_ADDR_SRINDY, _addrCPU_getStackRelativeIndirectIndexedY(cpu, mem)); break;
        case 0xf4: i_pea(cpu, mem); break;
        case 0xf5: i_sbc(cpu, mem, 2, 4, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0xf6: i_inc(cpu, mem, 2, 6, CPU_ADDR_DPX, _addrCPU_getDirectPageIndexedX(cpu, mem)); break;
        case 0xf7: i_sbc(cpu, mem, 2, 6, CPU_ADDR_INDDPLY, _addrCPU_getDirectPageIndirectLongIndexedY(cpu, mem)); break;
        case 0xf8: i_sed(cpu); break;
        case 0xf9: i_sbc(cpu, mem, 3, 4, CPU_ADDR_ABSY, _addrCPU_getAbsoluteIndexedY(cpu, mem)); break;
        case 0xfa: i_plx(cpu, mem); break;
        case 0xfb: i_xce(cpu); break;
        case 0xfc: i_jsr(cpu, mem, 8, CPU_ADDR_ABS, _addrCPU_getAbsoluteIndexedIndirectX(cpu, mem)); break;
        case 0xfd: i_sbc(cpu, mem, 3, 4, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0xfe: i_inc(cpu, mem, 3, 7, CPU_ADDR_ABSX, _addrCPU_getAbsoluteIndexedX(cpu, mem)); break;
        case 0xff: i_sbc(cpu, mem, 4, 5, CPU_ADDR_ABSLX, _addrCPU_getLongIndexedX(cpu, mem)); break;
        default:
            return CPU_ERR_UNKNOWN_OPCODE;
    }

    // Make sure opcode handling did not result in an invalid state
    if (cpu->P.CRASH == 1)
    {
        return CPU_ERR_CRASH;
    }

    // Handle any interrupts that are pending
    if (cpu->P.NMI)
    {
        cpu->P.NMI = 0;

        if (cpu->P.E)
        {
            _stackCPU_pushWord(cpu, mem, cpu->PC, CPU_ESTACK_ENABLE);
            _stackCPU_pushByte(cpu, mem, _cpu_get_sr(cpu) & 0xef); // B gets reset on the stack
            cpu->PC = mem[CPU_VEC_EMU_NMI];
            cpu->PC |= mem[CPU_VEC_EMU_NMI + 1] << 8;
            cpu->PBR = 0;
            cpu->cycles += 7;
        }
        else
        {
            _stackCPU_push24(cpu, mem, _cpu_get_effective_pc(cpu));
            _stackCPU_pushByte(cpu, mem, _cpu_get_sr(cpu));
            cpu->PC = mem[CPU_VEC_NATIVE_NMI];
            cpu->PC |= mem[CPU_VEC_NATIVE_NMI + 1] << 8;
            cpu->PBR = 0;
            cpu->cycles += 8;
        }

        cpu->P.D = 0; // Binary mode (65C02)
        // cpu->P.I = 1; // IRQ flag is not set: https://softpixel.com/~cwright/sianse/docs/65816NFO.HTM#7.00

        return CPU_ERR_OK;
    }
    if (cpu->P.IRQ && !cpu->P.I)
    {
        cpu->P.IRQ = 0;

        if (cpu->P.E)
        {
            _stackCPU_pushWord(cpu, mem, cpu->PC, CPU_ESTACK_ENABLE);
            _stackCPU_pushByte(cpu, mem, _cpu_get_sr(cpu) & 0xef); // B gets reset on the stack
            cpu->PC = mem[CPU_VEC_EMU_IRQ];
            cpu->PC |= mem[CPU_VEC_EMU_IRQ + 1] << 8;
            cpu->PBR = 0;
            cpu->cycles += 7;
        }
        else
        {
            _stackCPU_push24(cpu, mem, _cpu_get_effective_pc(cpu));
            _stackCPU_pushByte(cpu, mem, _cpu_get_sr(cpu));
            cpu->PC = mem[CPU_VEC_NATIVE_IRQ];
            cpu->PC |= mem[CPU_VEC_NATIVE_IRQ + 1] << 8;
            cpu->PBR = 0;
            cpu->cycles += 8;
        }

        cpu->P.D = 0; // Binary mode (65C02)
        cpu->P.I = 1;

        return CPU_ERR_OK;
    }


    return CPU_ERR_OK;
}
