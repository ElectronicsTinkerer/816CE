
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
    cpu->SP &= 0x00ff;
    cpu->SP |= 0x0100;
    cpu->X &= 0x00ff;
    cpu->Y &= 0x00ff;
    cpu->P.M = 1;
    cpu->P.XB = 1;
    cpu->P.D = 0;
    cpu->P.I = 1;
    cpu->P.E = 1;

    // Internal use only, tell the sim that the CPU just reset
    cpu->P.RST = 1;
    cpu->cycles = 0;

    return CPU_ERR_OK;
}

/**
 * Steps a CPU by one mchine cycle
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

    // Handle CPU reset
    if (cpu->P.RST)
    {
        cpu->PC = mem[CPU_VEC_RESET];
        return CPU_ERR_OK;
    }

    if (cpu->P.STP)
    {
        return CPU_ERR_STP;
    }

    // Fetch, decode, execute instruction
    switch (mem[cpu->PC])
    {
    // case 0xnn: status = i_cmp(cpu, mem, _addrCPU_relevantAddressingModeEffectiveAddressCalculation(stuff));
    case 0x00: i_brk(cpu, mem); break;

    case 0x02: i_cop(cpu, mem); break;

    case 0x08: i_php(cpu, mem); break;

    case 0x0b: i_phd(cpu, mem); break;

    case 0x10: i_bpl(cpu, mem); break;

    case 0x18: i_clc(cpu); break;

    case 0x1a: i_ina(cpu); break;
    case 0x1b: i_tcs(cpu); break;

    case 0x20: // JSR addr
        _stackCPU_pushWord(cpu, mem, _addr_add_val_bank_wrap(cpu->PC, 2), CPU_ESTACK_ENABLE);
        cpu->PC = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
        cpu->cycles += 6;
        break;

    case 0x22: // JSL/JSR long
        _stackCPU_push24(cpu, mem, _addr_add_val_bank_wrap(CPU_GET_EFFECTIVE_PC(cpu), 3));
        cpu->PBR = mem[_addr_add_val_bank_wrap(CPU_GET_EFFECTIVE_PC(cpu), 3)] & 0xff;
        cpu->PC = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
        cpu->cycles += 8;
        break;

    case 0x24: // BIT dp
    {
        int32_t addr = _addrCPU_getDirectPage(cpu, mem);
        if (cpu->P.E || (!cpu->P.E && cpu->P.M)) // 8-bit
        {
            uint8_t val = _get_mem_byte(mem, addr);
            cpu->P.Z = ((cpu->C & 0xff) & val) ? 0 : 1;
            cpu->P.N = (val & 0x80) ? 1 : 0;
            cpu->P.V = (val & 0x40) ? 1 : 0;
            cpu->cycles += 3;
            _cpu_update_pc(cpu, 2);
        }
        else // 16-bit
        {
            uint16_t val = ADDR_GET_MEM_DP_WORD(mem, addr);
            cpu->P.Z = ((cpu->C & 0xffff) & val) ? 0 : 1;
            cpu->P.N = (val & 0x8000) ? 1 : 0;
            cpu->P.V = (val & 0x4000) ? 1 : 0;
            cpu->cycles += 4;
            _cpu_update_pc(cpu, 2);
        }

        // If DL != 0, add a cycle
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
        }
            break;

        case 0x25: // AND dp
        {
            int32_t addr = _addrCPU_getDirectPage(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                cpu->C = (cpu->C & 0xff00) | ( (cpu->C & 0xff) & _get_mem_byte(mem, addr) );
                cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 3;
            }
            else // 16-bit
            {
                cpu->C = (cpu->C & 0xffff) & ADDR_GET_MEM_DP_WORD(mem, addr);
                cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xffff) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 4;
            }

            // If DL != 0, add a cycle
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
        }
            break;

        case 0x27: // AND [dp]
        {
            int32_t addr = _addrCPU_getDirectPageIndirectLong(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                cpu->C = (cpu->C & 0xff00) | ( (cpu->C & 0xff) & _get_mem_byte(mem, addr) );
                cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 6;
            }
            else // 16-bit
            {
                cpu->C = (cpu->C & 0xffff) & ADDR_GET_MEM_ABS_WORD(mem, addr);
                cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xffff) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 7;
            }

            // If DL != 0, add a cycle
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
        }
            break;

        case 0x28: i_plp(cpu, mem); break;

        case 0x29: // AND #const
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                cpu->C = (cpu->C & 0xff00) | ( (cpu->C & 0xff) & _cpu_get_immd_byte(cpu, mem));
                cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 2;
            }
            else // 16-bit
            {
                cpu->C = (cpu->C & 0xffff) & ADDR_GET_MEM_IMMD_WORD(cpu, mem);
                cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xffff) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 3;
            }
            break;

        case 0x2b: i_pld(cpu, mem); break;

        case 0x2c: // BIT addr
        {
            int32_t addr = _addrCPU_getAbsolute(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.M)) // 8-bit
            {
                uint8_t val = _get_mem_byte(mem, addr);
                cpu->P.Z = ((cpu->C & 0xff) & val) ? 0 : 1;
                cpu->P.N = (val & 0x80) ? 1 : 0;
                cpu->P.V = (val & 0x40) ? 1 : 0;
                cpu->cycles += 4;
                _cpu_update_pc(cpu, 3);
            }
            else // 16-bit
            {
                uint16_t val = ADDR_GET_MEM_ABS_WORD(mem, addr);
                cpu->P.Z = ((cpu->C & 0xffff) & val) ? 0 : 1;
                cpu->P.N = (val & 0x8000) ? 1 : 0;
                cpu->P.V = (val & 0x4000) ? 1 : 0;
                cpu->cycles += 5;
                _cpu_update_pc(cpu, 3);
            }
        }
            break;

        case 0x2d: // AND addr
        {
            int32_t addr = _addrCPU_getAbsolute(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                cpu->C = (cpu->C & 0xff00) | ( (cpu->C & 0xff) & _get_mem_byte(mem, addr) );
                cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 4;
            }
            else // 16-bit
            {
                cpu->C = (cpu->C & 0xffff) & ADDR_GET_MEM_ABS_WORD(mem, addr);
                cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xffff) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 5;
            }
        }
            break;

        case 0x2f: // AND long
        {
            int32_t addr = _addrCPU_getLong(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                cpu->C = (cpu->C & 0xff00) | ( (cpu->C & 0xff) & _get_mem_byte(mem, addr) );
                cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
                _cpu_update_pc(cpu, 4);
                cpu->cycles += 5;
            }
            else // 16-bit
            {
                cpu->C = (cpu->C & 0xffff) & ADDR_GET_MEM_ABS_WORD(mem, addr);
                cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xffff) ? 0 : 1;
                _cpu_update_pc(cpu, 4);
                cpu->cycles += 6;
            }
        }
            break;

        case 0x30: i_bmi(cpu, mem); break;

        case 0x32: // AND (dp)
        {
            int32_t addr = _addrCPU_getDirectPageIndirect(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                cpu->C = (cpu->C & 0xff00) | ( (cpu->C & 0xff) & _get_mem_byte(mem, addr) );
                cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 5;
            }
            else // 16-bit
            {
                cpu->C = (cpu->C & 0xffff) & ADDR_GET_MEM_ABS_WORD(mem, addr);
                cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xffff) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 6;
            }

            // If DL != 0, add a cycle
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
        }
            break;

        case 0x34: // BIT dp,X
        {
            int32_t addr = _addrCPU_getDirectPageIndexedX(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.M)) // 8-bit
            {
                uint8_t val = _get_mem_byte(mem, addr);
                cpu->P.Z = ((cpu->C & 0xff) & val) ? 0 : 1;
                cpu->P.N = (val & 0x80) ? 1 : 0;
                cpu->P.V = (val & 0x40) ? 1 : 0;
                cpu->cycles += 4;
                _cpu_update_pc(cpu, 2);
            }
            else // 16-bit
            {
                uint16_t val = ADDR_GET_MEM_DP_WORD(mem, addr);
                cpu->P.Z = ((cpu->C & 0xffff) & val) ? 0 : 1;
                cpu->P.N = (val & 0x8000) ? 1 : 0;
                cpu->P.V = (val & 0x4000) ? 1 : 0;
                cpu->cycles += 5;
                _cpu_update_pc(cpu, 2);
            }

            // If DL != 0, add a cycle
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
        }
            break;

        case 0x35: // AND dp,X
        {
            int32_t addr = _addrCPU_getDirectPageIndexedX(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                cpu->C = (cpu->C & 0xff00) | ( (cpu->C & 0xff) & _get_mem_byte(mem, addr) );
                cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 4;
            }
            else // 16-bit
            {
                cpu->C = (cpu->C & 0xffff) & ADDR_GET_MEM_DP_WORD(mem, addr);
                cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xffff) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 5;
            }

            // If DL != 0, add a cycle
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
        }
            break;

        case 0x38: i_sec(cpu); break;

        case 0x39: // AND addr,Y
        {
            int32_t addr = _addrCPU_getAbsoluteIndexedY(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                cpu->C = (cpu->C & 0xff00) | ( (cpu->C & 0xff) & _get_mem_byte(mem, addr) );
                cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 4;
            }
            else // 16-bit
            {
                cpu->C = (cpu->C & 0xffff) & ADDR_GET_MEM_ABS_WORD(mem, addr);
                cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xffff) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 5;
            }

            // Check if index crosses a page boundary
            if ( (addr & 0xff00) != ((addr - cpu->Y) & 0xff00) )
            {
                cpu->cycles += 1;
            }
        }
            break;

        case 0x3a: i_dea(cpu); break;
        case 0x3b: i_tsc(cpu); break;

        case 0x3c: // BIT addr,X
        {
            int32_t addr = _addrCPU_getAbsoluteIndexedX(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.M)) // 8-bit
            {
                uint8_t val = _get_mem_byte(mem, addr);
                cpu->P.Z = ((cpu->C & 0xff) & val) ? 0 : 1;
                cpu->P.N = (val & 0x80) ? 1 : 0;
                cpu->P.V = (val & 0x40) ? 1 : 0;
                cpu->cycles += 4;
                _cpu_update_pc(cpu, 3);
            }
            else // 16-bit
            {
                uint16_t val = ADDR_GET_MEM_ABS_WORD(mem, addr);
                cpu->P.Z = ((cpu->C & 0xffff) & val) ? 0 : 1;
                cpu->P.N = (val & 0x8000) ? 1 : 0;
                cpu->P.V = (val & 0x4000) ? 1 : 0;
                cpu->cycles += 5;
                _cpu_update_pc(cpu, 3);
            }

            // If page boundary is crossed, add a cycle
            if ( (ADDR_GET_MEM_IMMD_WORD(cpu, mem) & 0xff00) != (addr & 0xff00) )
            {
                cpu->cycles += 1;
            }
        }
            break;

        case 0x3d: // AND addr,X
        {
            int32_t addr = _addrCPU_getAbsoluteIndexedX(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                cpu->C = (cpu->C & 0xff00) | ( (cpu->C & 0xff) & _get_mem_byte(mem, addr) );
                cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 4;
            }
            else // 16-bit
            {
                cpu->C = (cpu->C & 0xffff) & ADDR_GET_MEM_ABS_WORD(mem, addr);
                cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xffff) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 5;
            }

            // Check if index crosses a page boundary
            if ( (addr & 0xff00) != ((addr - cpu->X) & 0xff00) )
            {
                cpu->cycles += 1;
            }
        }
            break;

        case 0x3f: // AND long,X
        {
            int32_t addr = _addrCPU_getLongIndexedX(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                cpu->C = (cpu->C & 0xff00) | ( (cpu->C & 0xff) & _get_mem_byte(mem, addr) );
                cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
                _cpu_update_pc(cpu, 4);
                cpu->cycles += 5;
            }
            else // 16-bit
            {
                cpu->C = (cpu->C & 0xffff) & ADDR_GET_MEM_ABS_WORD(mem, addr);
                cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
                cpu->P.Z = (cpu->C & 0xffff) ? 0 : 1;
                _cpu_update_pc(cpu, 4);
                cpu->cycles += 6;
            }
        }
            break;

        case 0x40: i_rti(cpu, mem); break;

        case 0x42: i_wdm(cpu); break;

        case 0x48: i_pha(cpu, mem); break;

        case 0x4b: i_phk(cpu, mem); break;

        case 0x4c: // JMP addr
            cpu->PC = _cpu_get_immd_word(cpu, mem);
            cpu->cycles += 3;
            break;

        case 0x50: i_bvc(cpu, mem); break;

        case 0x58: i_cli(cpu); break;

        case 0x5a: i_phy(cpu, mem); break;
        case 0x5b: i_tcd(cpu); break;

        case 0x5c: // JMP long
            cpu->PC = _get_mem_byte(mem, _addr_add_val_bank_wrap(CPU_GET_EFFECTIVE_PC(cpu), 3));
            cpu->PC = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
            cpu->cycles += 4;
            break;

        case 0x60: i_rts(cpu, mem); break;

        case 0x62: i_per(cpu, mem); break;

        case 0x64: // STZ dp
            mem[_addrCPU_getDirectPage(cpu, mem)] = 0;
            cpu->cycles += 3;
            if (!cpu->P.M) // 16-bit
            {
                mem[ _addr_add_val_bank_wrap(_addrCPU_getDirectPage(cpu, mem), 1)] = 0; // Bank wrapping
                cpu->cycles += 1;
            }
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            break;

        case 0x68: i_pla(cpu, mem); break;

        case 0x6b: i_rtl(cpu, mem); break;

        case 0x6c: // JMP (addr)
            cpu->PC = _addrCPU_mem_getAbsoluteIndirect(cpu, mem);
            cpu->cycles += 5;
            break;

        case 0x70: i_bvs(cpu, mem); break;

        case 0x74: // STZ dp,X
            mem[_addrCPU_getDirectPageIndexedX(cpu, mem)] = 0;
            cpu->cycles += 4;
            if (!cpu->P.M) // 16-bit
            {
                mem[_addr_add_val_bank_wrap(_addrCPU_getDirectPageIndexedX(cpu, mem), 1)] = 0; // Bank wrapping
                cpu->cycles += 1;
            }
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            break;

        case 0x78: i_sei(cpu); break;

        case 0x7a: i_ply(cpu, mem); break;
        case 0x7b: i_tdc(cpu); break;

        case 0x7c: // JMP (addr,X)
            cpu->PC = _addrCPU_mem_getAbsoluteIndexedIndirectX(cpu, mem);
            cpu->cycles += 6;
            break;

        case 0x80: i_bra(cpu, mem); break;

        case 0x82: i_brl(cpu, mem); break;

        case 0x84: // STY dp
            mem[_addrCPU_getDirectPage(cpu, mem)] = cpu->Y & 0xff;
            cpu->cycles += 3;
            if (!cpu->P.E && !cpu->P.XB) // 16-bit
            {
                mem[_addr_add_val_bank_wrap(_addrCPU_getDirectPage(cpu, mem), 1)] = (cpu->Y >> 8) & 0xff; // Bank wrapping
                cpu->cycles += 1;
            }
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            break;

        case 0x86: // STX dp
            mem[_addrCPU_getDirectPage(cpu, mem)] = cpu->X & 0xff;
            cpu->cycles += 3;
            if (!cpu->P.E && !cpu->P.XB) // 16-bit
            {
                mem[_addr_add_val_bank_wrap(_addrCPU_getDirectPage(cpu, mem), 1)] = (cpu->X >> 8) & 0xff; // Bank wrapping
                cpu->cycles += 1;
            }
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            break;

        case 0x88: i_dey(cpu); break;

        case 0x89: // BIT #const
            if (cpu->P.E || (!cpu->P.E && cpu->P.M)) // 8-bit
            {
                cpu->P.Z = ((cpu->C & 0xff) & _cpu_get_immd_byte(cpu, mem)) ? 0 : 1;
                cpu->cycles += 2;
                _cpu_update_pc(cpu, 2);
            }
            else // 16-bit
            {
                cpu->P.Z = ((cpu->C & 0xffff) & ADDR_GET_MEM_IMMD_WORD(cpu, mem)) ? 0 : 1;
                cpu->cycles += 3;
                _cpu_update_pc(cpu, 3);
            }
            break;

        case 0x8a: i_txa(cpu); break;

        case 0x8b: i_phb(cpu, mem); break;

        case 0x8c: // STY addr
            mem[_addrCPU_getAbsolute(cpu, mem)] = cpu->Y & 0xff;
            cpu->cycles += 4;
            if (!cpu->P.E && !cpu->P.XB) // 16-bit
            {
                mem[_addrCPU_getAbsolute(cpu, mem) + 1] = (cpu->Y >> 8) & 0xff; // No bank wrapping
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 3);
            break;

        case 0x8e: // STX addr
            mem[_addrCPU_getAbsolute(cpu, mem)] = cpu->X & 0xff;
            cpu->cycles += 4;
            if (!cpu->P.E && !cpu->P.XB) // 16-bit
            {
                mem[_addrCPU_getAbsolute(cpu, mem) + 1] = (cpu->X >> 8) & 0xff; // No bank wrapping
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 3);
            break;

        case 0x90: i_bcc(cpu, mem); break;

        case 0x94: // STY dp,X
            mem[_addrCPU_getDirectPageIndexedX(cpu, mem)] = cpu->Y & 0xff;
            cpu->cycles += 4;
            if (!cpu->P.E && !cpu->P.XB) // 16-bit
            {
                mem[_addr_add_val_bank_wrap(_addrCPU_getDirectPageIndexedX(cpu, mem), 1)] = (cpu->Y >> 8) & 0xff; // Bank wrapping
                cpu->cycles += 1;
            }
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            break;

        case 0x96: // STX dp,Y
            mem[_addrCPU_getDirectPageIndexedY(cpu, mem)] = cpu->X & 0xff;
            cpu->cycles += 4;
            if (!cpu->P.E && !cpu->P.XB) // 16-bit
            {
                mem[_addr_add_val_bank_wrap(_addrCPU_getDirectPageIndexedY(cpu, mem), 1)] = (cpu->X >> 8) & 0xff; // Bank wrapping
                cpu->cycles += 1;
            }
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            break;

        case 0x98: i_tya(cpu); break;

        case 0x9a: i_txs(cpu); break;
        case 0x9b: i_txy(cpu); break;

        case 0x9c: // STZ addr
            mem[_addrCPU_getAbsolute(cpu, mem)] = 0;
            cpu->cycles += 4;
            if (!cpu->P.M) // 16-bit
            {
                mem[_addrCPU_getAbsolute(cpu, mem) + 1] = 0; // No bank wrapping
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 3);
            break;

        case 0x9e: // STZ addr,x
            mem[_addrCPU_getAbsoluteIndexedX(cpu, mem)] = 0;
            cpu->cycles += 5;
            if (!cpu->P.M) // 16-bit
            {
                mem[_addrCPU_getAbsoluteIndexedX(cpu, mem) + 1] = 0; // No bank wrapping
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 3);
            break;

        case 0xa0: // LDY #const
        {
            if (cpu->P.E)
            {
                cpu->Y = _cpu_get_immd_byte(cpu, mem);
                cpu->P.Z = ((cpu->Y & 0xff) == 0);
                cpu->P.N = ((cpu->Y & 0x80) == 0x80);
            }
            else
            {
                if (cpu->P.XB)
                {
                    cpu->Y = _cpu_get_immd_byte(cpu, mem);
                    cpu->P.Z = ((cpu->Y & 0xff) == 0);
                    cpu->P.N = ((cpu->Y & 0x80) == 0x80);
                }
                else
                {
                    cpu->Y = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
                    cpu->P.Z = ((cpu->Y & 0xffff) == 0);
                    cpu->P.N = ((cpu->Y & 0x8000) == 0x8000);
                    _cpu_update_pc(cpu, 1);
                    cpu->cycles += 1;
                }
            }
            _cpu_update_pc(cpu, 2);
            cpu->cycles += 2;
        }
            break;

        case 0xa2: // LDX #const
        {
            if (cpu->P.E)
            {
                cpu->X = _cpu_get_immd_byte(cpu, mem);
                cpu->P.Z = ((cpu->X & 0xff) == 0);
                cpu->P.N = ((cpu->X & 0x80) == 0x80);
            }
            else
            {
                if (cpu->P.XB)
                {
                    cpu->X = _cpu_get_immd_byte(cpu, mem);
                    cpu->P.Z = ((cpu->X & 0xff) == 0);
                    cpu->P.N = ((cpu->X & 0x80) == 0x80);
                }
                else
                {
                    cpu->X = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
                    cpu->P.Z = ((cpu->X & 0xffff) == 0);
                    cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
                    _cpu_update_pc(cpu, 1);
                    cpu->cycles += 1;
                }
            }
            _cpu_update_pc(cpu, 2);
            cpu->cycles += 2;
        }
            break;

        case 0xa4: // LDY dp
        {
            int32_t addr = _addrCPU_getDirectPage(cpu, mem);

            if (cpu->P.E)
            {
                cpu->Y = _get_mem_byte(mem, addr);
                cpu->P.Z = ((cpu->Y & 0xff) == 0);
                cpu->P.N = ((cpu->Y & 0x80) == 0x80);
            }
            else
            {
                if (cpu->P.XB)
                {
                    cpu->Y = _get_mem_byte(mem, addr);
                    cpu->P.Z = ((cpu->Y & 0xff) == 0);
                    cpu->P.N = ((cpu->Y & 0x80) == 0x80);
                }
                else
                {
                    cpu->Y = ADDR_GET_MEM_DP_WORD(mem, addr);
                    cpu->P.Z = ((cpu->Y & 0xffff) == 0);
                    cpu->P.N = ((cpu->Y & 0x8000) == 0x8000);
                    cpu->cycles += 1;
                }
            }
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            cpu->cycles += 3;
        }
            break;

        case 0xa6: // LDX dp
        {
            int32_t addr = _addrCPU_getDirectPage(cpu, mem);

            if (cpu->P.E)
            {
                cpu->X = _get_mem_byte(mem, addr);
                cpu->P.Z = ((cpu->X & 0xff) == 0);
                cpu->P.N = ((cpu->X & 0x80) == 0x80);
            }
            else
            {
                if (cpu->P.XB)
                {
                    cpu->X = _get_mem_byte(mem, addr);
                    cpu->P.Z = ((cpu->X & 0xff) == 0);
                    cpu->P.N = ((cpu->X & 0x80) == 0x80);
                }
                else
                {
                    cpu->X = ADDR_GET_MEM_DP_WORD(mem, addr);
                    cpu->P.Z = ((cpu->X & 0xffff) == 0);
                    cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
                    cpu->cycles += 1;
                }
            }
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            cpu->cycles += 3;
        }
            break;

        case 0xa8: i_tay(cpu); break;

        case 0xaa: i_tax(cpu); break;
        case 0xab: i_plb(cpu, mem); break;

        case 0xac: // LDY addr
        {
            int32_t addr = _addrCPU_getAbsolute(cpu, mem);

            if (cpu->P.E)
            {
                cpu->Y = _get_mem_byte(mem, addr);
                cpu->P.Z = ((cpu->Y & 0xff) == 0);
                cpu->P.N = ((cpu->Y & 0x80) == 0x80);
            }
            else
            {
                if (cpu->P.XB)
                {
                    cpu->Y = _get_mem_byte(mem, addr);
                    cpu->P.Z = ((cpu->Y & 0xff) == 0);
                    cpu->P.N = ((cpu->Y & 0x80) == 0x80);
                }
                else
                {
                    cpu->Y = ADDR_GET_MEM_ABS_WORD(mem, addr);
                    cpu->P.Z = ((cpu->Y & 0xffff) == 0);
                    cpu->P.N = ((cpu->Y & 0x8000) == 0x8000);
                    cpu->cycles += 1;
                }
            }
            _cpu_update_pc(cpu, 3);
            cpu->cycles += 4;
        }
            break;

        case 0xae: // LDX addr
        {
            int32_t addr = _addrCPU_getAbsolute(cpu, mem);

            if (cpu->P.E)
            {
                cpu->X = _get_mem_byte(mem, addr);
                cpu->P.Z = ((cpu->X & 0xff) == 0);
                cpu->P.N = ((cpu->X & 0x80) == 0x80);
            }
            else
            {
                if (cpu->P.XB)
                {
                    cpu->X = _get_mem_byte(mem, addr);
                    cpu->P.Z = ((cpu->X & 0xff) == 0);
                    cpu->P.N = ((cpu->X & 0x80) == 0x80);
                }
                else
                {
                    cpu->X = ADDR_GET_MEM_ABS_WORD(mem, addr);
                    cpu->P.Z = ((cpu->X & 0xffff) == 0);
                    cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
                    cpu->cycles += 1;
                }
            }
            _cpu_update_pc(cpu, 3);
            cpu->cycles += 4;
        }
            break;

        case 0xb0: i_bcs(cpu, mem); break;

        case 0xb4: // LDY dp,X
        {
            int32_t addr = _addrCPU_getDirectPageIndexedX(cpu, mem);

            if (cpu->P.E)
            {
                cpu->Y = _get_mem_byte(mem, addr);
                cpu->P.Z = ((cpu->Y & 0xff) == 0);
                cpu->P.N = ((cpu->Y & 0x80) == 0x80);
            }
            else
            {
                if (cpu->P.XB)
                {
                    cpu->Y = _get_mem_byte(mem, addr);
                    cpu->P.Z = ((cpu->Y & 0xff) == 0);
                    cpu->P.N = ((cpu->Y & 0x80) == 0x80);
                }
                else
                {
                    cpu->Y = ADDR_GET_MEM_DP_WORD(mem, addr);
                    cpu->P.Z = ((cpu->Y & 0xffff) == 0);
                    cpu->P.N = ((cpu->Y & 0x8000) == 0x8000);
                    cpu->cycles += 1;
                }
            }
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            cpu->cycles += 4;
        }
            break;

        case 0xb6: // LDX dp,Y
        {
            int32_t addr = _addrCPU_getDirectPageIndexedY(cpu, mem);

            if (cpu->P.E)
            {
                cpu->X = _get_mem_byte(mem, addr);
                cpu->P.Z = ((cpu->X & 0xff) == 0);
                cpu->P.N = ((cpu->X & 0x80) == 0x80);
            }
            else
            {
                if (cpu->P.XB)
                {
                    cpu->X = _get_mem_byte(mem, addr);
                    cpu->P.Z = ((cpu->X & 0xff) == 0);
                    cpu->P.N = ((cpu->X & 0x80) == 0x80);
                }
                else
                {
                    cpu->X = ADDR_GET_MEM_DP_WORD(mem, addr);
                    cpu->P.Z = ((cpu->X & 0xffff) == 0);
                    cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
                    cpu->cycles += 1;
                }
            }
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            cpu->cycles += 4;
        }
            break;

        case 0xb8: i_clv(cpu); break;

        case 0xba: i_tsx(cpu); break;
        case 0xbb: i_tyx(cpu); break;

        case 0xbc: // LDY addr,X
        {
            int32_t addr = _addrCPU_getAbsoluteIndexedX(cpu, mem);

            if (cpu->P.E)
            {
                cpu->Y = _get_mem_byte(mem, addr);
                cpu->P.Z = ((cpu->Y & 0xff) == 0);
                cpu->P.N = ((cpu->Y & 0x80) == 0x80);
            }
            else
            {
                if (cpu->P.XB)
                {
                    cpu->Y = _get_mem_byte(mem, addr);
                    cpu->P.Z = ((cpu->Y & 0xff) == 0);
                    cpu->P.N = ((cpu->Y & 0x80) == 0x80);
                }
                else
                {
                    cpu->Y = ADDR_GET_MEM_ABS_WORD(mem, addr);
                    cpu->P.Z = ((cpu->Y & 0xffff) == 0);
                    cpu->P.N = ((cpu->Y & 0x8000) == 0x8000);
                    cpu->cycles += 1;
                }
            }

            // If page boundary is crossed, add a cycle
            if ( (ADDR_GET_MEM_IMMD_WORD(cpu, mem) & 0xff00) != (addr & 0xff00) )
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 3);
            cpu->cycles += 4;
        }
            break;

        case 0xbe: // LDX addr,Y
        {
            int32_t addr = _addrCPU_getAbsoluteIndexedY(cpu, mem);

            if (cpu->P.E)
            {
                cpu->X = _get_mem_byte(mem, addr);
                cpu->P.Z = ((cpu->X & 0xff) == 0);
                cpu->P.N = ((cpu->X & 0x80) == 0x80);
            }
            else
            {
                if (cpu->P.XB)
                {
                    cpu->X = _get_mem_byte(mem, addr);
                    cpu->P.Z = ((cpu->X & 0xff) == 0);
                    cpu->P.N = ((cpu->X & 0x80) == 0x80);
                }
                else
                {
                    cpu->X = ADDR_GET_MEM_ABS_WORD(mem, addr);
                    cpu->P.Z = ((cpu->X & 0xffff) == 0);
                    cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
                    cpu->cycles += 1;
                }
            }

            // If page boundary is crossed, add a cycle
            if ( (ADDR_GET_MEM_IMMD_WORD(cpu, mem) & 0xff00) != (addr & 0xff00) )
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 3);
            cpu->cycles += 4;
        }
            break;

        case 0xc0: // CPY #const
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                uint8_t res = ( (cpu->Y & 0xff) - _cpu_get_immd_byte(cpu, mem) ) & 0xff;
                cpu->P.N = (res & 0x80) ? 1 : 0;
                cpu->P.Z = res ? 0 : 1;
                cpu->P.C = ((cpu->Y & 0xff) < res) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 2;
            }
            else // 16-bit
            {
                uint16_t res = ((cpu->Y & 0xffff) - ADDR_GET_MEM_IMMD_WORD(cpu, mem)) & 0xffff;
                cpu->P.N = (res & 0x8000) ? 1 : 0;
                cpu->P.Z = res ? 0 : 1;
                cpu->P.C = ((cpu->Y & 0xffff) < res) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 3;
            }
            break;

        case 0xc2: i_rep(cpu, mem);
            break;

        case 0xc4: // CPY dp
        {
            int32_t addr = _addrCPU_getDirectPage(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                uint8_t res = ( (cpu->Y & 0xff) - _get_mem_byte(mem, addr) ) & 0xff;
                cpu->P.N = (res & 0x80) ? 1 : 0;
                cpu->P.Z = res ? 0 : 1;
                cpu->P.C = ((cpu->Y & 0xff) < res) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 3;
            }
            else // 16-bit
            {
                uint16_t res = ((cpu->Y & 0xffff) - ADDR_GET_MEM_DP_WORD(mem, addr) ) & 0xffff;
                cpu->P.N = (res & 0x8000) ? 1 : 0;
                cpu->P.Z = res ? 0 : 1;
                cpu->P.C = ((cpu->Y & 0xffff) < res) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 4;
            }

            // If DL != 0, add a cycle
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
        }
            break;

        case 0xc6: // DEC dp
        {
            int32_t addr = _addrCPU_getDirectPage(cpu, mem);
            if (cpu->P.E)
            {
                mem[addr] = (_get_mem_byte(mem, addr) - 1) & 0xff;
                cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
            }
            else
            {
                if (cpu->P.M)
                {
                    mem[addr] = (_get_mem_byte(mem, addr) - 1) & 0xff;
                    cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                    cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
                }
                else // 16-bit
                {
                    int32_t val = (ADDR_GET_MEM_DP_WORD(mem, addr) - 1) & 0xffff;
                    mem[addr] = val & 0xff;
                    mem[addr+1] = (val >> 8) & 0xff;
                    cpu->P.N = val & 0x8000 ? 1 : 0;
                    cpu->P.Z = val & 0xffff ? 0 : 1;
                    cpu->cycles += 2;
                }
            }
            if (cpu->D & 0xff) 
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            cpu->cycles += 5;
        }
            break;

        case 0xc8: i_iny(cpu); break;

        case 0xca: i_dex(cpu); break;
        case 0xcb: i_wai(cpu); break;

        case 0xcc: // CPY addr
        {
            int32_t addr = _addrCPU_getAbsolute(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                uint8_t res = ( (cpu->Y & 0xff) - _get_mem_byte(mem, addr) ) & 0xff;
                cpu->P.N = (res & 0x80) ? 1 : 0;
                cpu->P.Z = res ? 0 : 1;
                cpu->P.C = ((cpu->Y & 0xff) < res) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 4;
            }
            else // 16-bit
            {
                uint16_t res = ((cpu->Y & 0xffff) - ADDR_GET_MEM_ABS_WORD(mem, addr) ) & 0xffff;
                cpu->P.N = (res & 0x8000) ? 1 : 0;
                cpu->P.Z = res ? 0 : 1;
                cpu->P.C = ((cpu->Y & 0xffff) < res) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 5;
            }
        }
            break;

        case 0xce: // DEC addr
        {
            int32_t addr = _addrCPU_getAbsolute(cpu, mem);
            if (cpu->P.E)
            {
                mem[addr] = (_get_mem_byte(mem, addr) - 1) & 0xff;
                cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
            }
            else
            {
                if (cpu->P.M)
                {
                    mem[addr] = (_get_mem_byte(mem, addr) - 1) & 0xff;
                    cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                    cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
                }
                else // 16-bit
                {
                    int32_t val = (ADDR_GET_MEM_ABS_WORD(mem, addr) - 1) & 0xffff;
                    mem[addr] = val & 0xff;
                    mem[addr+1] = (val >> 8) & 0xff;
                    cpu->P.N = val & 0x8000 ? 1 : 0;
                    cpu->P.Z = val & 0xffff ? 0 : 1;
                    cpu->cycles += 2;
                }
            }
            _cpu_update_pc(cpu, 3);
            cpu->cycles += 6;
        }
            break;

        case 0xd0: i_bne(cpu, mem); break;

        case 0xd4: i_pei(cpu, mem); break;


        case 0xd6: // DEC dp,X
        {
            int32_t addr = _addrCPU_getDirectPageIndexedX(cpu, mem);
            if (cpu->P.E)
            {
                mem[addr] = (_get_mem_byte(mem, addr) - 1) & 0xff;
                cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
            }
            else
            {
                if (cpu->P.XB)
                {
                    mem[addr] = (_get_mem_byte(mem, addr) - 1) & 0xff;
                    cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                    cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
                }
                else // 16-bit
                {
                    int32_t val = (ADDR_GET_MEM_DP_WORD(mem, addr) - 1) & 0xffff;
                    mem[addr] = val & 0xff;
                    mem[addr+1] = (val >> 8) & 0xff;
                    cpu->P.N = val & 0x8000 ? 1 : 0;
                    cpu->P.Z = val & 0xffff ? 0 : 1;
                    cpu->cycles += 2;
                }
            }
            if (cpu->D & 0xff) 
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            cpu->cycles += 6;
        }
            break;


        case 0xd8: i_cld(cpu); break;

        case 0xda: i_phx(cpu, mem); break;
        case 0xdb: i_stp(cpu); break;

        case 0xde: // DEC addr,X
        {
            int32_t addr = _addrCPU_getAbsoluteIndexedX(cpu, mem);
            if (cpu->P.E)
            {
                mem[addr] = (_get_mem_byte(mem, addr) - 1) & 0xff;
                cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
            }
            else
            {
                if (cpu->P.XB)
                {
                    mem[addr] = (_get_mem_byte(mem, addr) - 1) & 0xff;
                    cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                    cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
                }
                else // 16-bit
                {
                    int32_t val = (ADDR_GET_MEM_ABS_WORD(mem, addr) - 1) & 0xffff;
                    mem[addr] = val & 0xff;
                    mem[addr + 1] = (val >> 8) & 0xff;
                    cpu->P.N = val & 0x8000 ? 1 : 0;
                    cpu->P.Z = val & 0xffff ? 0 : 1;
                    cpu->cycles += 2;
                }
            }
            _cpu_update_pc(cpu, 3);
            cpu->cycles += 7;
        }
            break;

        case 0xdc: // JMP [addr]
        {
            int32_t addr = _addrCPU_mem_getAbsoluteIndirectLong(cpu, mem);
            cpu->PBR = (addr & 0xff0000) >> 16;
            cpu->PC = addr & 0xffff;
            cpu->cycles += 6;
        }
            break;

        case 0xe0: // CPX #const
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                uint8_t res = ( (cpu->X & 0xff) - _cpu_get_immd_byte(cpu, mem) ) & 0xff;
                cpu->P.N = (res & 0x80) ? 1 : 0;
                cpu->P.Z = res ? 0 : 1;
                cpu->P.C = ((cpu->X & 0xff) < res) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 2;
            }
            else // 16-bit
            {
                uint16_t res = ((cpu->X & 0xffff) - ADDR_GET_MEM_IMMD_WORD(cpu, mem)) & 0xffff;
                cpu->P.N = (res & 0x8000) ? 1 : 0;
                cpu->P.Z = res ? 0 : 1;
                cpu->P.C = ((cpu->X & 0xffff) < res) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 3;
            }
            break;

        case 0xe2: i_sep(cpu, mem); break;

        case 0xe4: // CPX dp
        {
            int32_t addr = _addrCPU_getDirectPage(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                uint8_t res = ( (cpu->X & 0xff) - _get_mem_byte(mem, addr) ) & 0xff;
                cpu->P.N = (res & 0x80) ? 1 : 0;
                cpu->P.Z = res ? 0 : 1;
                cpu->P.C = ((cpu->X & 0xff) < res) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 3;
            }
            else // 16-bit
            {
                uint16_t res = ((cpu->X & 0xffff) - ADDR_GET_MEM_DP_WORD(mem, addr) ) & 0xffff;
                cpu->P.N = (res & 0x8000) ? 1 : 0;
                cpu->P.Z = res ? 0 : 1;
                cpu->P.C = ((cpu->X & 0xffff) < res) ? 0 : 1;
                _cpu_update_pc(cpu, 2);
                cpu->cycles += 4;
            }

            // If DL != 0, add a cycle
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
        }
            break;

        case 0xe6: // INC dp
        {
            int32_t addr = _addrCPU_getDirectPage(cpu, mem);
            if (cpu->P.E)
            {
                mem[addr] = (_get_mem_byte(mem, addr) + 1) & 0xff;
                cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
            }
            else
            {
                if (cpu->P.M)
                {
                    mem[addr] = (_get_mem_byte(mem, addr) + 1) & 0xff;
                    cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                    cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
                }
                else // 16-bit
                {
                    int32_t val = (ADDR_GET_MEM_DP_WORD(mem, addr) + 1) & 0xffff;
                    mem[addr] = val & 0xff;
                    mem[addr+1] = (val >> 8) & 0xff;
                    cpu->P.N = val & 0x8000 ? 1 : 0;
                    cpu->P.Z = val & 0xffff ? 0 : 1;
                    cpu->cycles += 2;
                }
            }
            if (cpu->D & 0xff) 
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            cpu->cycles += 5;
        }
            break;

        case 0xe8: i_inx(cpu); break;

        case 0xea: i_nop(cpu); break;
        case 0xeb: i_xba(cpu); break;

        case 0xec: // CPX addr
        {
            int32_t addr = _addrCPU_getAbsolute(cpu, mem);
            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                uint8_t res = ( (cpu->X & 0xff) - _get_mem_byte(mem, addr) ) & 0xff;
                cpu->P.N = (res & 0x80) ? 1 : 0;
                cpu->P.Z = res ? 0 : 1;
                cpu->P.C = ((cpu->X & 0xff) < res) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 4;
            }
            else // 16-bit
            {
                uint16_t res = ((cpu->X & 0xffff) - ADDR_GET_MEM_ABS_WORD(mem, addr) ) & 0xffff;
                cpu->P.N = (res & 0x8000) ? 1 : 0;
                cpu->P.Z = res ? 0 : 1;
                cpu->P.C = ((cpu->X & 0xffff) < res) ? 0 : 1;
                _cpu_update_pc(cpu, 3);
                cpu->cycles += 5;
            }
        }
            break;

        case 0xee: // INC addr
        {
            int32_t addr = _addrCPU_getAbsolute(cpu, mem);
            if (cpu->P.E)
            {
                mem[addr] = (_get_mem_byte(mem, addr) + 1) & 0xff;
                cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
            }
            else
            {
                if (cpu->P.M)
                {
                    mem[addr] = (_get_mem_byte(mem, addr) + 1) & 0xff;
                    cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                    cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
                }
                else // 16-bit
                {
                    int32_t val = (ADDR_GET_MEM_ABS_WORD(mem, addr) + 1) & 0xffff;
                    mem[addr] = val & 0xff;
                    mem[addr+1] = (val >> 8) & 0xff; // No bank wrap
                    cpu->P.N = val & 0x8000 ? 1 : 0;
                    cpu->P.Z = val & 0xffff ? 0 : 1;
                    cpu->cycles += 2;
                }
            }
            _cpu_update_pc(cpu, 3);
            cpu->cycles += 6;
        }
            break;

        case 0xf0: i_beq(cpu, mem); break;

        case 0xf4: i_pea(cpu, mem); break;

        case 0xf6: // INC dp,X
        {
            int32_t addr = _addrCPU_getDirectPageIndexedX(cpu, mem);
            if (cpu->P.E)
            {
                mem[addr] = (_get_mem_byte(mem, addr) + 1) & 0xff;
                cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
            }
            else
            {
                if (cpu->P.M)
                {
                    mem[addr] = (_get_mem_byte(mem, addr) + 1) & 0xff;
                    cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                    cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
                }
                else // 16-bit
                {
                    int32_t val = (ADDR_GET_MEM_DP_WORD(mem, addr) + 1) & 0xffff;
                    mem[addr] = val & 0xff;
                    mem[addr+1] = (val >> 8) & 0xff;
                    cpu->P.N = val & 0x8000 ? 1 : 0;
                    cpu->P.Z = val & 0xffff ? 0 : 1;
                    cpu->cycles += 2;
                }
            }
            if (cpu->D & 0xff) 
            {
                cpu->cycles += 1;
            }
            _cpu_update_pc(cpu, 2);
            cpu->cycles += 6;
        }
            break;

        case 0xf8: i_sed(cpu); break;

        case 0xfa: i_plx(cpu, mem); break;
        case 0xfb: i_xce(cpu); break;

        case 0xfc: // JSR (addr,X)
            _stackCPU_pushWord(cpu, mem, _addr_add_val_bank_wrap(cpu->PC, 2), CPU_ESTACK_DISABLE);
            cpu->PC = _addrCPU_mem_getAbsoluteIndexedIndirectX(cpu, mem);
            cpu->cycles += 8;
            break;

        case 0xfe: // INC addr,X
        {
            int32_t addr = _addrCPU_getAbsoluteIndexedX(cpu, mem);
            if (cpu->P.E)
            {
                mem[addr] = (_get_mem_byte(mem, addr) + 1) & 0xff;
                cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
            }
            else
            {
                if (cpu->P.M)
                {
                    mem[addr] = (_get_mem_byte(mem, addr) + 1) & 0xff;
                    cpu->P.N = _get_mem_byte(mem, addr) & 0x80 ? 1 : 0;
                    cpu->P.Z = _get_mem_byte(mem, addr) & 0xff ? 0 : 1;
                }
                else // 16-bit
                {
                    int32_t val = (ADDR_GET_MEM_ABS_WORD(mem, addr) + 1) & 0xffff;
                    mem[addr] = val & 0xff;
                    mem[addr+1] = (val >> 8) & 0xff;
                    cpu->P.N = val & 0x8000 ? 1 : 0;
                    cpu->P.Z = val & 0xffff ? 0 : 1;
                    cpu->cycles += 2;
                }
            }
            _cpu_update_pc(cpu, 3);
            cpu->cycles += 7;
        }
            break;

        default:
            return CPU_ERR_UNKNOWN_OPCODE;
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
