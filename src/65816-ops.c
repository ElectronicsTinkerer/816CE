
#include "65816-ops.h"

void i_and(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_DPX ||
        mode == CPU_ADDR_DPY)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            cpu->C = (cpu->C & 0xff00) | ((cpu->C & 0xff) & _get_mem_byte(mem, addr));
            cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
            cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
        }
        else // 16-bit
        {
            cpu->C = cpu->C & _get_mem_word_bank_wrap(mem, addr);
            cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
            cpu->P.Z = cpu->C ? 0 : 1;
            cpu->cycles += 1;
        }

        // If DL != 0, add a cycle
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_DPIND || mode == CPU_ADDR_DPINDL ||
             mode == CPU_ADDR_ABS || mode == CPU_ADDR_ABSX ||
             mode == CPU_ADDR_ABSY || mode == CPU_ADDR_ABSL ||
             mode == CPU_ADDR_ABSLX || mode == CPU_ADDR_DPINDX ||
             mode == CPU_ADDR_INDDPY || mode == CPU_ADDR_INDDPLY ||
             mode == CPU_ADDR_SRINDY)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            cpu->C = (cpu->C & 0xff00) | ((cpu->C & 0xff) & _get_mem_byte(mem, addr));
            cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
            cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
        }
        else // 16-bit
        {
            cpu->C = cpu->C & _get_mem_word(mem, addr);
            cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
            cpu->P.Z = cpu->C ? 0 : 1;
            cpu->cycles += 1;
        }

        if (mode == CPU_ADDR_ABSX || mode == CPU_ADDR_ABSY ||
            mode == CPU_ADDR_INDDPY || mode == CPU_ADDR_INDDPLY)
        {
            // Check if index crosses a page boundary
            if ((addr & 0xff00) != ((addr - cpu->Y) & 0xff00))
            {
                cpu->cycles += 1;
            }
        }
        if (mode == CPU_ADDR_DPIND || mode == CPU_ADDR_DPINDL ||
            mode == CPU_ADDR_DPINDX || mode == CPU_ADDR_INDDPY ||
            mode == CPU_ADDR_INDDPLY)
        {
            // If DL != 0, add a cycle
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
        }
    }
    else if (mode == CPU_ADDR_IMMD || mode == CPU_ADDR_SR)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            cpu->C = (cpu->C & 0xff00) | ((cpu->C & 0xff) & _get_mem_byte(mem, addr));
            cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
            cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
        }
        else // 16-bit
        {
            cpu->C = cpu->C & _get_mem_word_bank_wrap(mem, addr);
            cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
            cpu->P.Z = cpu->C ? 0 : 1;
            cpu->cycles += 1;
        }
    }
    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_asl(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    uint16_t post_data = 0;
    uint16_t pre_data = 0;
    switch (mode)
    {
        case CPU_ADDR_DP:
        case CPU_ADDR_DPX:
            pre_data = _get_mem_word_bank_wrap(mem, addr);

            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                post_data = ((pre_data << 1) & 0xff);
                _set_mem_byte(mem, addr, (uint8_t)post_data);
            }
            else // 16-bit
            {
                post_data = pre_data << 1;
                _set_mem_word_bank_wrap(mem, addr, post_data);
                cpu->cycles += 2;
            }

            // If DL != 0, add a cycle
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
            break;
        case CPU_ADDR_ABS:
        case CPU_ADDR_ABSX:
            pre_data = _get_mem_word(mem, addr);

            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                post_data = ((pre_data << 1) & 0xff);
                _set_mem_byte(mem, addr, (uint8_t)post_data);
            }
            else // 16-bit
            {
                post_data = pre_data << 1;
                _set_mem_word(mem, addr, post_data);
                cpu->cycles += 2;
            }
            break;
        case CPU_ADDR_IMPD:
            pre_data = cpu->C;

            if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
            {
                post_data = ((pre_data << 1) & 0xff);
                cpu->C = (cpu->C & 0xff00) | post_data;
            }
            else // 16-bit
            {
                post_data = pre_data << 1;
                cpu->C = post_data;
            }
            break;
    }

    if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
    {
        cpu->C = (pre_data & 0x80) ? 1 : 0;
        cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
        cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
    }
    else // 16-bit
    {
        cpu->P.C = (pre_data & 0x8000) ? 1 : 0;
        cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
        cpu->P.Z = cpu->C ? 0 : 1;
    }

    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_bcc(CPU_t *cpu, memory_t *mem)
{
    if (!cpu->P.C)
    {
        int32_t new_PC = _addrCPU_getRelative8(cpu, mem);
        cpu->cycles += 1;

        // Add a cycle if page boundary crossed in emulation mode
        if (cpu->P.E && ((new_PC & 0xff00) != (cpu->PC & 0xff00)))
        {
            cpu->cycles += 1;
        }
        cpu->PC = new_PC;
    }
    else
    {
        _cpu_update_pc(cpu, 2);
    }
    cpu->cycles += 2;
}

void i_bcs(CPU_t *cpu, memory_t *mem)
{
    if (cpu->P.C)
    {
        int32_t new_PC = _addrCPU_getRelative8(cpu, mem);
        cpu->cycles += 1;

        // Add a cycle if page boundary crossed in emulation mode
        if (cpu->P.E && ((new_PC & 0xff00) != (cpu->PC & 0xff00)))
        {
            cpu->cycles += 1;
        }
        cpu->PC = new_PC;
    }
    else
    {
        _cpu_update_pc(cpu, 2);
    }
    cpu->cycles += 2;
}

void i_beq(CPU_t *cpu, memory_t *mem)
{
    if (cpu->P.Z)
    {
        int32_t new_PC = _addrCPU_getRelative8(cpu, mem);
        cpu->cycles += 1;

        // Add a cycle if page boundary crossed in emulation mode
        if (cpu->P.E && ((new_PC & 0xff00) != (cpu->PC & 0xff00)))
        {
            cpu->cycles += 1;
        }
        cpu->PC = new_PC;
    }
    else
    {
        _cpu_update_pc(cpu, 2);
    }
    cpu->cycles += 2;
}

void i_bit(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_DPX)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.M)) // 8-bit
        {
            uint8_t val = _get_mem_byte(mem, addr);
            cpu->P.Z = ((cpu->C & 0xff) & val) ? 0 : 1;
            cpu->P.N = (val & 0x80) ? 1 : 0;
            cpu->P.V = (val & 0x40) ? 1 : 0;
        }
        else // 16-bit
        {
            uint16_t val = _get_mem_word_bank_wrap(mem, addr);
            cpu->P.Z = (cpu->C & val) ? 0 : 1;
            cpu->P.N = (val & 0x8000) ? 1 : 0;
            cpu->P.V = (val & 0x4000) ? 1 : 0;
            cpu->cycles += 1;
        }

        // If DL != 0, add a cycle
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_ABS || mode == CPU_ADDR_ABSX)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.M)) // 8-bit
        {
            uint8_t val = _get_mem_byte(mem, addr);
            cpu->P.Z = ((cpu->C & 0xff) & val) ? 0 : 1;
            cpu->P.N = (val & 0x80) ? 1 : 0;
            cpu->P.V = (val & 0x40) ? 1 : 0;
        }
        else // 16-bit
        {
            uint16_t val = _get_mem_word(mem, addr);
            cpu->P.Z = (cpu->C & val) ? 0 : 1;
            cpu->P.N = (val & 0x8000) ? 1 : 0;
            cpu->P.V = (val & 0x4000) ? 1 : 0;
            cpu->cycles += 1;
        }

        // If page boundary is crossed, add a cycle
        if (mode == CPU_ADDR_ABSX && (_cpu_get_immd_word(cpu, mem) & 0xff00) != (addr & 0xff00))
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_IMMD)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.M)) // 8-bit
        {
            uint8_t val = _get_mem_byte(mem, addr);
            cpu->P.Z = ((cpu->C & 0xff) & val) ? 0 : 1; // Only Z for immediate addressing
        }
        else // 16-bit
        {
            uint16_t val = _get_mem_word_bank_wrap(mem, addr);
            cpu->P.Z = (cpu->C & val) ? 0 : 1;
            cpu->cycles += 1;
            size += 1;
        }
    }
    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_bmi(CPU_t *cpu, memory_t *mem)
{
    if (cpu->P.N)
    {
        int32_t new_PC = _addrCPU_getRelative8(cpu, mem);
        cpu->cycles += 1;

        // Add a cycle if page boundary crossed in emulation mode
        if (cpu->P.E && ((new_PC & 0xff00) != (cpu->PC & 0xff00)))
        {
            cpu->cycles += 1;
        }
        cpu->PC = new_PC;
    }
    else
    {
        _cpu_update_pc(cpu, 2);
    }
    cpu->cycles += 2;
}

void i_bne(CPU_t *cpu, memory_t *mem)
{
    if (!cpu->P.Z)
    {
        int32_t new_PC = _addrCPU_getRelative8(cpu, mem);
        cpu->cycles += 1;

        // Add a cycle if page boundary crossed in emulation mode
        if (cpu->P.E && ((new_PC & 0xff00) != (cpu->PC & 0xff00)))
        {
            cpu->cycles += 1;
        }
        cpu->PC = new_PC;
    }
    else
    {
        _cpu_update_pc(cpu, 2);
    }
    cpu->cycles += 2;
}

void i_bpl(CPU_t *cpu, memory_t *mem)
{
    if (!cpu->P.N)
    {
        int32_t new_PC = _addrCPU_getRelative8(cpu, mem);
        cpu->cycles += 1;

        // Add a cycle if page boundary crossed in emulation mode
        if (cpu->P.E && ((new_PC & 0xff00) != (cpu->PC & 0xff00)))
        {
            cpu->cycles += 1;
        }
        cpu->PC = new_PC;
    }
    else
    {
        _cpu_update_pc(cpu, 2);
    }
    cpu->cycles += 2;
}

void i_bra(CPU_t *cpu, memory_t *mem)
{
    uint16_t new_PC = _addrCPU_getRelative8(cpu, mem);
    cpu->cycles += 3;

    // Add a cycle if page boundary crossed in emulation mode
    if (cpu->P.E && ((new_PC & 0xff00) != (cpu->PC & 0xff00)))
    {
        cpu->cycles += 1;
    }
    cpu->PC = new_PC;
}

void i_brk(CPU_t *cpu, memory_t *mem)
{
    _cpu_update_pc(cpu, 2);

    if (cpu->P.E)
    {
        _stackCPU_pushWord(cpu, mem, cpu->PC, CPU_ESTACK_ENABLE);
        _stackCPU_pushByte(cpu, mem, _cpu_get_sr(cpu) | 0x10); // B flag is set for BRK in emulation mode
        cpu->PC = _get_mem_byte(mem, CPU_VEC_EMU_IRQ);
        cpu->PC |= _get_mem_byte(mem, CPU_VEC_EMU_IRQ + 1) << 8;
        cpu->PBR = 0;
        cpu->cycles += 7;
    }
    else
    {
        _stackCPU_push24(cpu, mem, _cpu_get_effective_pc(cpu));
        _stackCPU_pushByte(cpu, mem, _cpu_get_sr(cpu));
        cpu->PC = _get_mem_byte(mem, CPU_VEC_NATIVE_BRK);
        cpu->PC |= _get_mem_byte(mem, CPU_VEC_NATIVE_BRK + 1) << 8;
        cpu->PBR = 0;
        cpu->cycles += 8;
    }

    cpu->P.D = 0; // Binary mode (65C02)
    cpu->P.I = 1;
}

void i_brl(CPU_t *cpu, memory_t *mem)
{
    cpu->PC = _addrCPU_getRelative16(cpu, mem);
    cpu->cycles += 4;
}

void i_bvc(CPU_t *cpu, memory_t *mem)
{
    if (!cpu->P.V)
    {
        int32_t new_PC = _addrCPU_getRelative8(cpu, mem);
        cpu->cycles += 1;

        // Add a cycle if page boundary crossed in emulation mode
        if (cpu->P.E && ((new_PC & 0xff00) != (cpu->PC & 0xff00)))
        {
            cpu->cycles += 1;
        }
        cpu->PC = new_PC;
    }
    else
    {
        _cpu_update_pc(cpu, 2);
    }
    cpu->cycles += 2;
}

void i_bvs(CPU_t *cpu, memory_t *mem)
{
    if (cpu->P.V)
    {
        int32_t new_PC = _addrCPU_getRelative8(cpu, mem);
        cpu->cycles += 1;

        // Add a cycle if page boundary crossed in emulation mode
        if (cpu->P.E && ((new_PC & 0xff00) != (cpu->PC & 0xff00)))
        {
            cpu->cycles += 1;
        }
        cpu->PC = new_PC;
    }
    else
    {
        _cpu_update_pc(cpu, 2);
    }
    cpu->cycles += 2;
}

void i_clc(CPU_t *cpu)
{
    cpu->P.C = 0;
    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_cld(CPU_t *cpu)
{
    cpu->P.D = 0;
    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_cli(CPU_t *cpu)
{
    cpu->P.I = 0;
    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_clv(CPU_t *cpu)
{
    cpu->P.V = 0;
    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_cop(CPU_t *cpu, memory_t *mem)
{
    _cpu_update_pc(cpu, 2);

    if (cpu->P.E)
    {
        _stackCPU_pushWord(cpu, mem, cpu->PC, CPU_ESTACK_ENABLE);
        _stackCPU_pushByte(cpu, mem, _cpu_get_sr(cpu) & 0xef); // ??? Unknown: the state of the B flag in ISR for COP (assumed to be 0)
        cpu->PC = _get_mem_byte(mem, CPU_VEC_EMU_COP);
        cpu->PC |= _get_mem_byte(mem, CPU_VEC_EMU_COP + 1) << 8;
        cpu->PBR = 0;
        cpu->cycles += 7;
    }
    else
    {
        _stackCPU_push24(cpu, mem, _cpu_get_effective_pc(cpu));
        _stackCPU_pushByte(cpu, mem, _cpu_get_sr(cpu));
        cpu->PC = _get_mem_byte(mem, CPU_VEC_NATIVE_COP);
        cpu->PC |= _get_mem_byte(mem, CPU_VEC_NATIVE_COP + 1) << 8;
        cpu->PBR = 0;
        cpu->cycles += 8;
    }

    cpu->P.D = 0; // Binary mode (65C02)
    cpu->P.I = 1;
}

void i_cpx(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_IMMD)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            uint8_t res = (cpu->X & 0xff) - _get_mem_byte(mem, addr);
            cpu->P.N = (res & 0x80) ? 1 : 0;
            cpu->P.Z = res ? 0 : 1;
            cpu->P.C = ((cpu->X & 0xff) < res) ? 0 : 1;
            
        }
        else // 16-bit
        {
            uint16_t res = _get_mem_word_bank_wrap(mem, addr);
            res = cpu->X - res;
            cpu->P.N = (res & 0x8000) ? 1 : 0;
            cpu->P.Z = res ? 0 : 1;
            cpu->P.C = (cpu->X < res) ? 0 : 1;
            cpu->cycles += 1;
            if (mode == CPU_ADDR_IMMD)
            {
                size += 1; // One extra byte in operand
            }
        }
        // If DL != 0, add a cycle
        if (mode == CPU_ADDR_DP && cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_ABS)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            uint8_t res = (cpu->X & 0xff) - _get_mem_byte(mem, addr);
            cpu->P.N = (res & 0x80) ? 1 : 0;
            cpu->P.Z = res ? 0 : 1;
            cpu->P.C = ((cpu->X & 0xff) < res) ? 0 : 1;
            
        }
        else // 16-bit
        {
            uint16_t res = cpu->X - _get_mem_word(mem, addr);
            cpu->P.N = (res & 0x8000) ? 1 : 0;
            cpu->P.Z = res ? 0 : 1;
            cpu->P.C = (cpu->X < res) ? 0 : 1;
            cpu->cycles += 1;
        }
    }
    _cpu_update_pc(cpu, size);
    cpu->cycles += cycles;
}

void i_cpy(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_IMMD)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            uint8_t res = (cpu->Y & 0xff) - _get_mem_byte(mem, addr);
            cpu->P.N = (res & 0x80) ? 1 : 0;
            cpu->P.Z = res ? 0 : 1;
            cpu->P.C = ((cpu->Y & 0xff) < res) ? 0 : 1;
        }
        else // 16-bit
        {
            uint16_t res = _get_mem_word_bank_wrap(mem, addr);
            res = cpu->Y - res;
            cpu->P.N = (res & 0x8000) ? 1 : 0;
            cpu->P.Z = res ? 0 : 1;
            cpu->P.C = (cpu->Y < res) ? 0 : 1;
            cpu->cycles += 1;
            if (mode == CPU_ADDR_IMMD)
            {
                size += 1; // One extra byte in operand
            }
        }
        // If DL != 0, add a cycle
        if (mode == CPU_ADDR_DP && cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_ABS)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            uint8_t res = (cpu->Y & 0xff) - _get_mem_byte(mem, addr);
            cpu->P.N = (res & 0x80) ? 1 : 0;
            cpu->P.Z = res ? 0 : 1;
            cpu->P.C = ((cpu->Y & 0xff) < res) ? 0 : 1;
        }
        else // 16-bit
        {
            uint16_t res = cpu->Y - _get_mem_word(mem, addr);
            cpu->P.N = (res & 0x8000) ? 1 : 0;
            cpu->P.Z = res ? 0 : 1;
            cpu->P.C = (cpu->Y < res) ? 0 : 1;
            cpu->cycles += 1;
        }
    }
    _cpu_update_pc(cpu, size);
    cpu->cycles += cycles;
}

void i_dea(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->C = ((cpu->C - 1) & 0xff) | (cpu->C & 0xff00);
        cpu->P.N = cpu->C & 0x80 ? 1 : 0;
        cpu->P.Z = cpu->C & 0xff ? 0 : 1;
    }
    else
    {
        if (cpu->P.M)
        {
            cpu->C = ((cpu->C - 1) & 0xff) | (cpu->C & 0xff00);
            cpu->P.N = cpu->C & 0x80 ? 1 : 0;
            cpu->P.Z = cpu->C & 0xff ? 0 : 1;
        }
        else // 16-bit
        {
            cpu->C = (cpu->C - 1) & 0xffff;
            cpu->P.N = cpu->C & 0x8000 ? 1 : 0;
            cpu->P.Z = cpu->C ? 0 : 1;
        }
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_dec(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_DPX)
    {
        if (cpu->P.E)
        {
            uint8_t val = _get_mem_byte(mem, addr) - 1;
            _set_mem_byte(mem, addr, val);
            cpu->P.N = val & 0x80 ? 1 : 0;
            cpu->P.Z = val ? 0 : 1;
        }
        else
        {
            if (cpu->P.M)
            {
                uint8_t val = _get_mem_byte(mem, addr) - 1;
                _set_mem_byte(mem, addr, val);
                cpu->P.N = val & 0x80 ? 1 : 0;
                cpu->P.Z = val ? 0 : 1;
            }
            else // 16-bit
            {
                uint32_t addr_high = _addr_add_val_bank_wrap(addr, 1);
                uint16_t val = _get_mem_byte(mem, addr);
                val |= _get_mem_byte(mem, addr_high) << 8;
                val -= 1;
                _set_mem_word_bank_wrap(mem, addr, val);
                cpu->P.N = val & 0x8000 ? 1 : 0;
                cpu->P.Z = val ? 0 : 1;
                cpu->cycles += 2;
            }
        }
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_ABS || mode == CPU_ADDR_ABSX)
    {
        if (cpu->P.E)
        {
            uint8_t val = _get_mem_byte(mem, addr) - 1;
            _set_mem_byte(mem, addr, val);
            cpu->P.N = val & 0x80 ? 1 : 0;
            cpu->P.Z = val ? 0 : 1;
        }
        else
        {
            if (cpu->P.M)
            {
                uint8_t val = _get_mem_byte(mem, addr) - 1;
                _set_mem_byte(mem, addr, val);
                cpu->P.N = val & 0x80 ? 1 : 0;
                cpu->P.Z = val ? 0 : 1;
            }
            else // 16-bit
            {
                uint16_t val = _get_mem_word(mem, addr) - 1;
                _set_mem_word(mem, addr, val);
                cpu->P.N = val & 0x8000 ? 1 : 0;
                cpu->P.Z = val ? 0 : 1;
                cpu->cycles += 2;
            }
        }
    }

    _cpu_update_pc(cpu, size);
    cpu->cycles += cycles;
}

void i_dex(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->X = (cpu->X - 1) & 0xff;
        cpu->P.N = cpu->X & 0x80 ? 1 : 0;
        cpu->P.Z = cpu->X & 0xff ? 0 : 1;
    }
    else
    {
        if (cpu->P.XB)
        {
            cpu->X = (cpu->X - 1) & 0xff;
            cpu->P.N = cpu->X & 0x80 ? 1 : 0;
            cpu->P.Z = cpu->X & 0xff ? 0 : 1;
        }
        else // 16-bit
        {
            cpu->X = (cpu->X - 1) & 0xffff;
            cpu->P.N = cpu->X & 0x8000 ? 1 : 0;
            cpu->P.Z = cpu->X ? 0 : 1;
        }
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_dey(CPU_t *cpu)
{
    if (cpu->P.E || (!cpu->P.E && cpu->P.XB))
    {
        cpu->Y = (cpu->Y - 1) & 0xff;
        cpu->P.N = cpu->Y & 0x80 ? 1 : 0;
        cpu->P.Z = cpu->Y & 0xff ? 0 : 1;
    }
    else // 16-bit
    {
        cpu->Y = (cpu->Y - 1) & 0xffff;
        cpu->P.N = cpu->Y & 0x8000 ? 1 : 0;
        cpu->P.Z = cpu->Y ? 0 : 1;
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_eor(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_DPX ||
        mode == CPU_ADDR_DPY)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            cpu->C = (cpu->C & 0xff00) | ((cpu->C & 0xff) ^ _get_mem_byte(mem, addr));
        }
        else // 16-bit
        {
            cpu->C = cpu->C ^ _get_mem_word_bank_wrap(mem, addr);
        }

        // If DL != 0, add a cycle
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_DPIND || mode == CPU_ADDR_DPINDL ||
             mode == CPU_ADDR_ABS || mode == CPU_ADDR_ABSX ||
             mode == CPU_ADDR_ABSY || mode == CPU_ADDR_ABSL ||
             mode == CPU_ADDR_ABSLX || mode == CPU_ADDR_DPINDX ||
             mode == CPU_ADDR_INDDPY || mode == CPU_ADDR_INDDPLY ||
             mode == CPU_ADDR_SRINDY)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            cpu->C = (cpu->C & 0xff00) | ((cpu->C & 0xff) ^ _get_mem_byte(mem, addr));
        }
        else // 16-bit
        {
            cpu->C = cpu->C ^ _get_mem_word(mem, addr);
        }

        if (mode == CPU_ADDR_ABSX || mode == CPU_ADDR_ABSY ||
            mode == CPU_ADDR_INDDPY || mode == CPU_ADDR_INDDPLY)
        {
            // Check if index crosses a page boundary
            if ((addr & 0xff00) != ((addr - cpu->Y) & 0xff00))
            {
                cpu->cycles += 1;
            }
        }
        if (mode == CPU_ADDR_DPIND || mode == CPU_ADDR_DPINDL ||
            mode == CPU_ADDR_DPINDX || mode == CPU_ADDR_INDDPY ||
            mode == CPU_ADDR_INDDPLY)
        {
            // If DL != 0, add a cycle
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
        }
    }
    else if (mode == CPU_ADDR_IMMD || mode == CPU_ADDR_SR)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            cpu->C = (cpu->C & 0xff00) | ((cpu->C & 0xff) ^ _get_mem_byte(mem, addr));
        }
        else // 16-bit
        {
            cpu->C = cpu->C ^ _get_mem_word_bank_wrap(mem, addr);
        }
    }

    if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
    {
        cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
        cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
    }
    else // 16-bit
    {
        cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
        cpu->P.Z = cpu->C ? 0 : 1;
        cpu->cycles += 1;
    }
    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_ina(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->C = ((cpu->C + 1) & 0xff) | (cpu->C & 0xff00);
        cpu->P.N = cpu->C & 0x80 ? 1 : 0;
        cpu->P.Z = cpu->C & 0xff ? 0 : 1;
    }
    else
    {
        if (cpu->P.M)
        {
            cpu->C = ((cpu->C + 1) & 0xff) | (cpu->C & 0xff00);
            cpu->P.N = cpu->C & 0x80 ? 1 : 0;
            cpu->P.Z = cpu->C & 0xff ? 0 : 1;
        }
        else // 16-bit
        {
            cpu->C = (cpu->C + 1) & 0xffff;
            cpu->P.N = cpu->C & 0x8000 ? 1 : 0;
            cpu->P.Z = cpu->C ? 0 : 1;
        }
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_inc(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_DPX)
    {
        if (cpu->P.E)
        {
            uint8_t val = _get_mem_byte(mem, addr) + 1;
            _set_mem_byte(mem, addr, val);
            cpu->P.N = val & 0x80 ? 1 : 0;
            cpu->P.Z = val ? 0 : 1;
        }
        else
        {
            if (cpu->P.M)
            {
                uint8_t val = _get_mem_byte(mem, addr) + 1;
                _set_mem_byte(mem, addr, val);
                cpu->P.N = val & 0x80 ? 1 : 0;
                cpu->P.Z = val ? 0 : 1;
            }
            else // 16-bit
            {
                uint32_t addr_high = _addr_add_val_bank_wrap(addr, 1);
                uint16_t val = _get_mem_byte(mem, addr);
                val |= _get_mem_byte(mem, addr_high) << 8;
                val += 1;
                _set_mem_word_bank_wrap(mem, addr, val);
                cpu->P.N = val & 0x8000 ? 1 : 0;
                cpu->P.Z = val ? 0 : 1;
                cpu->cycles += 2;
            }
        }
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_ABS || mode == CPU_ADDR_ABSX)
    {
        if (cpu->P.E)
        {
            uint8_t val = _get_mem_byte(mem, addr) + 1;
            _set_mem_byte(mem, addr, val);
            cpu->P.N = val & 0x80 ? 1 : 0;
            cpu->P.Z = val ? 0 : 1;
        }
        else
        {
            if (cpu->P.M)
            {
                uint8_t val = _get_mem_byte(mem, addr) + 1;
                _set_mem_byte(mem, addr, val);
                cpu->P.N = val & 0x80 ? 1 : 0;
                cpu->P.Z = val ? 0 : 1;
            }
            else // 16-bit
            {
                uint16_t val = _get_mem_word(mem, addr) + 1;
                _set_mem_word(mem, addr, val);
                cpu->P.N = val & 0x8000 ? 1 : 0;
                cpu->P.Z = val ? 0 : 1;
                cpu->cycles += 2;
            }
        }
    }
  
    _cpu_update_pc(cpu, size);
    cpu->cycles += cycles;
}

void i_inx(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->X = (cpu->X + 1) & 0xff;
        cpu->P.N = cpu->X & 0x80 ? 1 : 0;
        cpu->P.Z = cpu->X & 0xff ? 0 : 1;
    }
    else
    {
        if (cpu->P.XB)
        {
            cpu->X = (cpu->X + 1) & 0xff;
            cpu->P.N = cpu->X & 0x80 ? 1 : 0;
            cpu->P.Z = cpu->X & 0xff ? 0 : 1;
        }
        else // 16-bit
        {
            cpu->X = (cpu->X + 1) & 0xffff;
            cpu->P.N = cpu->X & 0x8000 ? 1 : 0;
            cpu->P.Z = cpu->X ? 0 : 1;
        }
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_iny(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->Y = (cpu->Y + 1) & 0xff;
        cpu->P.N = cpu->Y & 0x80 ? 1 : 0;
        cpu->P.Z = cpu->Y & 0xff ? 0 : 1;
    }
    else
    {
        if (cpu->P.XB)
        {
            cpu->Y = (cpu->Y + 1) & 0xff;
            cpu->P.N = cpu->Y & 0x80 ? 1 : 0;
            cpu->P.Z = cpu->Y & 0xff ? 0 : 1;
        }
        else // 16-bit
        {
            cpu->Y = (cpu->Y + 1) & 0xffff;
            cpu->P.N = cpu->Y & 0x8000 ? 1 : 0;
            cpu->P.Z = cpu->Y ? 0 : 1;
        }
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_jmp(CPU_t *cpu, memory_t *mem, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_ABSL)
    {
        cpu->PBR = (addr >> 16) & 0xff;
    }
    cpu->PC = addr & 0xffff;
    cpu->cycles += cycles;
}

void i_jsr(CPU_t *cpu, memory_t *mem, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    _stackCPU_pushWord(cpu, mem, _addr_add_val_bank_wrap(cpu->PC, 2), CPU_ESTACK_ENABLE);
    cpu->PC = addr;
    cpu->cycles += cycles;
}

void i_jsl(CPU_t *cpu, memory_t *mem, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    uint32_t ret_addr = _addr_add_val_bank_wrap(_cpu_get_effective_pc(cpu), 3);
    _stackCPU_push24(cpu, mem, ret_addr);
    cpu->PBR = _get_mem_byte(mem, (addr >> 16) & 0xff);
    cpu->PC = addr & 0xffff;
    cpu->cycles += cycles;
}

void i_lda(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    switch (mode)
    {
    case CPU_ADDR_DP:
    case CPU_ADDR_DPX:
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
        /* Fallthrough! */
    case CPU_ADDR_IMMD:
    case CPU_ADDR_SR:
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB))
        {
            cpu->C = (cpu->C & 0xff00) | _get_mem_byte(mem, addr);
        }
        else
        {
            cpu->C = _get_mem_word_bank_wrap(mem, addr);
        }
        break;

    case CPU_ADDR_INDDPY:
        // If page boundary is crossed, add a cycle
        // getDirectPage() since this is the base address before Y is added
        if ((_addrCPU_getDirectPage(cpu, mem) & 0xff00) != (addr & 0xff00))
        {
            cpu->cycles += 1;
        }
        /* Fallthrough! */
    case CPU_ADDR_DPIND:
    case CPU_ADDR_DPINDL:
    case CPU_ADDR_DPINDX:
    case CPU_ADDR_INDDPLY:
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
        /* Fallthrough! */
    case CPU_ADDR_ABS:
    case CPU_ADDR_ABSL:
    case CPU_ADDR_ABSLX:
    case CPU_ADDR_SRINDY:
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB))
        {
            cpu->C = (cpu->C & 0xff00) | _get_mem_byte(mem, addr);
        }
        else
        {
            cpu->C = _get_mem_word(mem, addr);
        }
        break;

    case CPU_ADDR_ABSX:
    case CPU_ADDR_ABSY:
        // If page boundary is crossed, add a cycle
        if ((_cpu_get_immd_word(cpu, mem) & 0xff00) != (addr & 0xff00))
        {
            cpu->cycles += 1;
        }
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB))
        {
            cpu->C = (cpu->C & 0xff00) | _get_mem_byte(mem, addr);
        }
        else
        {
            cpu->C = _get_mem_word(mem, addr);
        }
        break;
    }

    if (cpu->P.E || (!cpu->P.E && cpu->P.XB))
    {
        cpu->P.Z = ((cpu->C & 0xff) == 0);
        cpu->P.N = ((cpu->C & 0x80) == 0x80);
    }
    else
    {
        cpu->P.Z = (cpu->C == 0);
        cpu->P.N = ((cpu->C & 0x8000) == 0x8000);
        cpu->cycles += 1;
    }

    _cpu_update_pc(cpu, size);
    cpu->cycles += cycles;
}

void i_ldx(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_DPY)
    {
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
                cpu->X = _get_mem_word_bank_wrap(mem, addr);
                cpu->P.Z = (cpu->X == 0);
                cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
                cpu->cycles += 1;
            }
        }
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_ABS || mode == CPU_ADDR_ABSY)
    {
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
                cpu->X = _get_mem_word(mem, addr);
                cpu->P.Z = (cpu->X == 0);
                cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
                cpu->cycles += 1;
            }
        }

        // If page boundary is crossed, add a cycle
        if (mode == CPU_ADDR_ABSY && (_cpu_get_immd_word(cpu, mem) & 0xff00) != (addr & 0xff00))
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_IMMD)
    {
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
                cpu->X = _get_mem_word_bank_wrap(mem, addr);
                cpu->P.Z = (cpu->X == 0);
                cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
                size += 1;
                cpu->cycles += 1;
            }
        }
    }
    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_ldy(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_DPX)
    {
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
                cpu->Y = _get_mem_word_bank_wrap(mem, addr);
                cpu->P.Z = (cpu->Y == 0);
                cpu->P.N = ((cpu->Y & 0x8000) == 0x8000);
                cpu->cycles += 1;
            }
        }
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_ABS || mode == CPU_ADDR_ABSX)
    {
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
                cpu->Y = _get_mem_word(mem, addr);
                cpu->P.Z = (cpu->Y == 0);
                cpu->P.N = ((cpu->Y & 0x8000) == 0x8000);
                cpu->cycles += 1;
            }
        }

        // If page boundary is crossed, add a cycle
        if (mode == CPU_ADDR_ABSX && (_cpu_get_immd_word(cpu, mem) & 0xff00) != (addr & 0xff00))
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_IMMD)
    {
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
                cpu->Y =  _get_mem_word_bank_wrap(mem, addr);
                cpu->P.Z = (cpu->Y == 0);
                cpu->P.N = ((cpu->Y & 0x8000) == 0x8000);
                size += 1;
                cpu->cycles += 1;
            }
        }
    }
    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_lsr(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    uint16_t post_data = 0;
    uint16_t pre_data = 0;
    switch (mode)
    {
    case CPU_ADDR_DP:
    case CPU_ADDR_DPX:
        pre_data = _get_mem_word_bank_wrap(mem, addr);

        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            post_data = ((pre_data >> 1) & 0xff);
            _set_mem_byte(mem, addr, (uint8_t)post_data);
        }
        else // 16-bit
        {
            post_data = pre_data >> 1;
            _set_mem_word_bank_wrap(mem, addr, post_data);
            cpu->cycles += 2;
        }

        // If DL != 0, add a cycle
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
        break;
    case CPU_ADDR_ABS:
    case CPU_ADDR_ABSX:
        pre_data = _get_mem_word(mem, addr);

        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            post_data = ((pre_data >> 1) & 0xff);
            _set_mem_byte(mem, addr, (uint8_t)post_data);
        }
        else // 16-bit
        {
            post_data = pre_data >> 1;
            _set_mem_word(mem, addr, post_data);
            cpu->cycles += 2;
        }
        break;
    case CPU_ADDR_IMPD:
        pre_data = cpu->C;

        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            post_data = ((pre_data >> 1) & 0xff);
            cpu->C = (cpu->C & 0xff00) | post_data;
        }
        else // 16-bit
        {
            post_data = pre_data << 1;
            cpu->C = post_data;
        }
        break;
    }

    if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
    {
        cpu->C = (pre_data & 0x80) ? 1 : 0;
        cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
        cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
    }
    else // 16-bit
    {
        cpu->P.C = (pre_data & 0x8000) ? 1 : 0;
        cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
        cpu->P.Z = cpu->C ? 0 : 1;
    }

    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_mvn(CPU_t *cpu, memory_t *mem)
{
    uint32_t operand_addr = _addrCPU_getImmediate(cpu, mem);

    // Read operands to get banks
    uint8_t dst_bank = _get_mem_byte(mem, operand_addr);
    uint8_t src_bank = _get_mem_byte(mem, _addr_add_val_bank_wrap(operand_addr, 1));

    // Calculate full addresses
    uint32_t dst_addr = (dst_bank << 16) | cpu->Y;
    uint32_t src_addr = (src_bank << 16) | cpu->X;

    // Perform copy
    uint8_t tmp = _get_mem_byte(mem, src_addr);
    _set_mem_byte(mem, dst_addr, tmp);

    // Update regs for next byte
    cpu->Y += 1;
    cpu->X += 1;
    cpu->DBR = dst_bank;

    cpu->C -= 1;

    // Are we done yet?
    if (cpu->C == (uint16_t)-1)
    {
        // Done, move to next instruction
        _cpu_update_pc(cpu, 3);
    }
    cpu->cycles += 7; // 7 cycles per byte moved
}

void i_mvp(CPU_t *cpu, memory_t *mem)
{
    uint32_t operand_addr = _addrCPU_getImmediate(cpu, mem);

    // Read operands to get banks
    uint8_t dst_bank = _get_mem_byte(mem, operand_addr);
    uint8_t src_bank = _get_mem_byte(mem, _addr_add_val_bank_wrap(operand_addr, 1));

    // Calculate full addresses
    uint32_t dst_addr = (dst_bank << 16) | cpu->Y;
    uint32_t src_addr = (src_bank << 16) | cpu->X;

    // Perform copy
    uint8_t tmp = _get_mem_byte(mem, src_addr);
    _set_mem_byte(mem, dst_addr, tmp);

    // Update regs for next byte
    cpu->Y -= 1;
    cpu->X -= 1;
    cpu->DBR = dst_bank;

    cpu->C -= 1;

    // Are we done yet?
    if (cpu->C == (uint16_t)-1)
    {
        // Done, move to next instruction
        _cpu_update_pc(cpu, 3);
    }
    cpu->cycles += 7; // 7 cycles per byte moved
}

void i_nop(CPU_t *cpu)
{
    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_ora(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_DPX ||
        mode == CPU_ADDR_DPY)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            cpu->C = (cpu->C & 0xff00) | ((cpu->C & 0xff) | _get_mem_byte(mem, addr));
        }
        else // 16-bit
        {
            cpu->C = cpu->C | _get_mem_word_bank_wrap(mem, addr);
        }

        // If DL != 0, add a cycle
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_DPIND || mode == CPU_ADDR_DPINDL ||
             mode == CPU_ADDR_ABS || mode == CPU_ADDR_ABSX ||
             mode == CPU_ADDR_ABSY || mode == CPU_ADDR_ABSL ||
             mode == CPU_ADDR_ABSLX || mode == CPU_ADDR_DPINDX ||
             mode == CPU_ADDR_INDDPY || mode == CPU_ADDR_INDDPLY ||
             mode == CPU_ADDR_SRINDY)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            cpu->C = (cpu->C & 0xff00) | ((cpu->C & 0xff) | _get_mem_byte(mem, addr));
        }
        else // 16-bit
        {
            cpu->C = cpu->C | _get_mem_word(mem, addr);
        }

        if (mode == CPU_ADDR_ABSX || mode == CPU_ADDR_ABSY ||
            mode == CPU_ADDR_INDDPY || mode == CPU_ADDR_INDDPLY)
        {
            // Check if index crosses a page boundary
            if ((addr & 0xff00) != ((addr - cpu->Y) & 0xff00))
            {
                cpu->cycles += 1;
            }
        }
        if (mode == CPU_ADDR_DPIND || mode == CPU_ADDR_DPINDL ||
            mode == CPU_ADDR_DPINDX || mode == CPU_ADDR_INDDPY ||
            mode == CPU_ADDR_INDDPLY)
        {
            // If DL != 0, add a cycle
            if (cpu->D & 0xff)
            {
                cpu->cycles += 1;
            }
        }
    }
    else if (mode == CPU_ADDR_IMMD || mode == CPU_ADDR_SR)
    {
        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            cpu->C = (cpu->C & 0xff00) | ((cpu->C & 0xff) | _get_mem_byte(mem, addr));
        }
        else // 16-bit
        {
            cpu->C = cpu->C | _get_mem_word_bank_wrap(mem, addr);
        }
    }

    if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
    {
        cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
        cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
    }
    else // 16-bit
    {
        cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
        cpu->P.Z = cpu->C ? 0 : 1;
        cpu->cycles += 1;
    }
    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_tax(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->X = cpu->C & 0xff;
        cpu->P.Z = ((cpu->X & 0xff) == 0);
        cpu->P.N = ((cpu->X & 0x80) == 0x80);
    }
    else
    {
        if (cpu->P.XB)
        {
            cpu->X = cpu->C & 0xff;
            cpu->P.Z = ((cpu->X & 0xff) == 0);
            cpu->P.N = ((cpu->X & 0x80) == 0x80);
        }
        else // 16-bit X
        {
            cpu->X = cpu->C;
            cpu->P.Z = (cpu->X == 0);
            cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
        }
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_tay(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->Y = cpu->C & 0xff;
        cpu->P.Z = ((cpu->Y & 0xff) == 0);
        cpu->P.N = ((cpu->Y & 0x80) == 0x80);
    }
    else
    {
        if (cpu->P.XB)
        {
            cpu->Y = cpu->C & 0xff;
            cpu->P.Z = ((cpu->Y & 0xff) == 0);
            cpu->P.N = ((cpu->X & 0x80) == 0x80);
        }
        else // 16-bit X
        {
            cpu->Y = cpu->C;
            cpu->P.Z = (cpu->Y == 0);
            cpu->P.N = ((cpu->Y & 0x8000) == 0x8000);
        }
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_tcs(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->SP = (cpu->C & 0xff) | 0x0100;
    }
    else // 16-bit transfer
    {
        cpu->SP = cpu->C;
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_tcd(CPU_t *cpu)
{
    // 16-bit transfer
    cpu->D = cpu->C;
    cpu->P.Z = (cpu->D == 0);
    cpu->P.N = ((cpu->D & 0x8000) == 0x8000);

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_tdc(CPU_t *cpu)
{
    // 16-bit transfer
    cpu->C = cpu->D;
    cpu->P.Z = (cpu->C == 0);
    cpu->P.N = ((cpu->C & 0x8000) == 0x8000);

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_tsc(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->C = (cpu->SP & 0xff) | 0x0100;
    }
    else // 16-bit transfer
    {
        cpu->C = cpu->SP;
    }

    cpu->P.N = ((cpu->C & 0x8000) == 0x8000);
    cpu->P.Z = (cpu->C == 0);

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_tsx(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->X = cpu->SP & 0xff;
        cpu->P.Z = ((cpu->X & 0xff) == 0);
        cpu->P.N = ((cpu->X & 0x80) == 0x80);
    }
    else
    {
        if (cpu->P.XB)
        {
            cpu->X = cpu->SP & 0xff;
            cpu->P.Z = ((cpu->X & 0xff) == 0);
            cpu->P.N = ((cpu->X & 0x80) == 0x80);
        }
        else
        {
            cpu->X = cpu->SP & 0xffff;
            cpu->P.Z = (cpu->X == 0);
            cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
        }
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_txa(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->C = cpu->X & 0xff;
        cpu->P.Z = ((cpu->C & 0xff) == 0);
        cpu->P.N = ((cpu->C & 0x80) == 0x80);
    }
    else
    {
        if (cpu->P.M) // 8-bit A and 8/16-bit X
        {
            cpu->C = (cpu->X & 0xff) | (cpu->C & 0xff00);
            cpu->P.Z = ((cpu->C & 0xff) == 0);
            cpu->P.N = ((cpu->C & 0x80) == 0x80);
        }
        else if (cpu->P.XB && !cpu->P.M) // 8-bit X, 16-bit A
        {
            cpu->C = cpu->X & 0xff;
            cpu->P.Z = ((cpu->C & 0xff) == 0);
            cpu->P.N = ((cpu->C & 0x80) == 0x80);
        }
        else // 16-bit A and X
        {
            cpu->C = cpu->X;
            cpu->P.Z = (cpu->C == 0);
            cpu->P.N = ((cpu->C & 0x8000) == 0x8000);
        }
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_txs(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->SP = (cpu->X & 0xff) | 0x0100;
    }
    else
    {
        if (cpu->P.XB)
        {
            cpu->SP = (cpu->X & 0xff); // Zero high byte of SP
        }
        else // 16-bit X
        {
            cpu->SP = cpu->X;
        }
    }
    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_txy(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->Y = cpu->X & 0xff;
        cpu->P.Z = ((cpu->Y & 0xff) == 0);
        cpu->P.N = ((cpu->Y & 0x80) == 0x80);
    }
    else
    {
        if (cpu->P.XB)
        {
            cpu->Y = cpu->X & 0xff;
            cpu->P.Z = ((cpu->Y & 0xff) == 0);
            cpu->P.N = ((cpu->Y & 0x80) == 0x80);
        }
        else // 16-bit
        {
            cpu->Y = cpu->X;
            cpu->P.Z = (cpu->Y == 0);
            cpu->P.N = ((cpu->Y & 0x8000) == 0x8000);
        }
    }
    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_tya(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->C = cpu->Y & 0xff;
        cpu->P.Z = ((cpu->C & 0xff) == 0);
        cpu->P.N = ((cpu->C & 0x80) == 0x80);
    }
    else
    {
        if (cpu->P.M) // 8-bit A and 8/16-bit X
        {
            cpu->C = (cpu->Y & 0xff) | (cpu->C & 0xff00);
            cpu->P.Z = ((cpu->C & 0xff) == 0);
            cpu->P.N = ((cpu->C & 0x80) == 0x80);
        }
        else if (cpu->P.XB && !cpu->P.M) // 8-bit X, 16-bit A
        {
            cpu->C = cpu->Y & 0xff;
            cpu->P.Z = ((cpu->C & 0xff) == 0);
            cpu->P.N = ((cpu->C & 0x80) == 0x80);
        }
        else // 16-bit A and X
        {
            cpu->C = cpu->Y;
            cpu->P.Z = (cpu->C == 0);
            cpu->P.N = ((cpu->C & 0x8000) == 0x8000);
        }
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_tyx(CPU_t *cpu)
{
    if (cpu->P.E)
    {
        cpu->X = cpu->Y & 0xff;
        cpu->P.Z = ((cpu->X & 0xff) == 0);
        cpu->P.N = ((cpu->X & 0x80) == 0x80);
    }
    else
    {
        if (cpu->P.XB)
        {
            cpu->X = cpu->Y & 0xff;
            cpu->P.Z = ((cpu->X & 0xff) == 0);
            cpu->P.N = ((cpu->X & 0x80) == 0x80);
        }
        else // 16-bit
        {
            cpu->X = cpu->Y;
            cpu->P.Z = (cpu->X == 0);
            cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
        }
    }
    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_pea(CPU_t *cpu, memory_t *mem)
{
    _stackCPU_pushWord(cpu, mem, _cpu_get_immd_word(cpu, mem), CPU_ESTACK_DISABLE);
    cpu->cycles += 5;
    _cpu_update_pc(cpu, 3);
}

void i_pei(CPU_t *cpu, memory_t *mem)
{
    uint32_t addr_dp = _addr_add_val_bank_wrap((cpu->D & 0xffff), _cpu_get_immd_byte(cpu, mem));
    uint16_t addr_ind = _get_mem_byte(mem, addr_dp);
    addr_ind |= _get_mem_byte(mem, _addr_add_val_bank_wrap(addr_dp, 1));
    _stackCPU_pushWord(cpu, mem, addr_ind, CPU_ESTACK_DISABLE);

    _cpu_update_pc(cpu, 2);
    cpu->cycles += 6;
    if (cpu->D & 0xff) // Add a cycle if DL != 0
    {
        cpu->cycles += 1;
    }
}

void i_per(CPU_t *cpu, memory_t *mem)
{
    int16_t displacement = _cpu_get_immd_word(cpu, mem);
    _cpu_update_pc(cpu, 3);
    _stackCPU_pushWord(cpu, mem, _addr_add_val_bank_wrap(cpu->PC, displacement), CPU_ESTACK_DISABLE);

    cpu->cycles += 6;
}

void i_pha(CPU_t *cpu, memory_t *mem)
{
    if (cpu->P.E || (!cpu->P.E && cpu->P.M)) // 8-bit A
    {
        _stackCPU_pushByte(cpu, mem, cpu->C);
        cpu->cycles += 3;
    }
    else // 16-bit A
    {
        _stackCPU_pushWord(cpu, mem, cpu->C, CPU_ESTACK_ENABLE);
        cpu->cycles += 4;
    }

    _cpu_update_pc(cpu, 1);
}

void i_phb(CPU_t *cpu, memory_t *mem)
{
    _stackCPU_pushByte(cpu, mem, cpu->DBR);
    cpu->cycles += 3;
    _cpu_update_pc(cpu, 1);
}

void i_phk(CPU_t *cpu, memory_t *mem)
{
    _stackCPU_pushByte(cpu, mem, cpu->PBR);
    cpu->cycles += 3;
    _cpu_update_pc(cpu, 1);
}

void i_phd(CPU_t *cpu, memory_t *mem)
{
    _stackCPU_pushWord(cpu, mem, cpu->D, CPU_ESTACK_DISABLE);
    cpu->cycles += 4;
    _cpu_update_pc(cpu, 1);
}

void i_php(CPU_t *cpu, memory_t *mem)
{
    _stackCPU_pushByte(cpu, mem, _cpu_get_sr(cpu));
    cpu->cycles += 3;
    _cpu_update_pc(cpu, 1);
}

void i_phx(CPU_t *cpu, memory_t *mem)
{
    if (cpu->P.E)
    {
        _stackCPU_pushByte(cpu, mem, cpu->X);
        cpu->cycles += 3;
    }
    else
    {
        if (cpu->P.XB) // 8-bit X
        {
            _stackCPU_pushByte(cpu, mem, cpu->X);
            cpu->cycles += 3;
        }
        else // 16-bit X
        {
            _stackCPU_pushWord(cpu, mem, cpu->X, CPU_ESTACK_ENABLE);
            cpu->cycles += 4;
        }
    }

    _cpu_update_pc(cpu, 1);
}

void i_phy(CPU_t *cpu, memory_t *mem)
{
    if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit X
    {
        _stackCPU_pushByte(cpu, mem, cpu->Y);
        cpu->cycles += 3;
    }
    else // 16-bit X
    {
        _stackCPU_pushWord(cpu, mem, cpu->Y, CPU_ESTACK_ENABLE);
        cpu->cycles += 4;
    }

    _cpu_update_pc(cpu, 1);
}

void i_pla(CPU_t *cpu, memory_t *mem)
{
    if (cpu->P.E)
    {
        cpu->C = _stackCPU_popByte(cpu, mem, CPU_ESTACK_ENABLE);
        cpu->cycles += 4;
        cpu->P.Z = ((cpu->C & 0xff) == 0);
        cpu->P.N = ((cpu->C & 0x80) == 0x80);
    }
    else
    {
        if (cpu->P.M)
        {
            cpu->C = _stackCPU_popByte(cpu, mem, CPU_ESTACK_ENABLE);
            cpu->cycles += 4;
            cpu->P.Z = ((cpu->C & 0xff) == 0);
            cpu->P.N = ((cpu->C & 0x80) == 0x80);
        }
        else // 16-bit A
        {
            cpu->C = _stackCPU_popWord(cpu, mem, CPU_ESTACK_ENABLE);
            cpu->cycles += 5;
            cpu->P.Z = (cpu->C == 0);
            cpu->P.N = ((cpu->C & 0x8000) == 0x8000);
        }
    }

    _cpu_update_pc(cpu, 1);
}

void i_plb(CPU_t *cpu, memory_t *mem)
{
    cpu->DBR = _stackCPU_popByte(cpu, mem, CPU_ESTACK_DISABLE);
    cpu->cycles += 4;
    cpu->P.Z = (cpu->DBR == 0);
    cpu->P.N = ((cpu->DBR & 0x80) == 0x80);

    _cpu_update_pc(cpu, 1);
}

void i_pld(CPU_t *cpu, memory_t *mem)
{
    cpu->D = _stackCPU_popWord(cpu, mem, CPU_ESTACK_DISABLE);
    cpu->cycles += 5;
    cpu->P.Z = (cpu->D == 0);
    cpu->P.N = ((cpu->D & 0x8000) == 0x8000);

    _cpu_update_pc(cpu, 1);
}

void i_plp(CPU_t *cpu, memory_t *mem)
{
    uint8_t sr = _cpu_get_sr(cpu);
    uint8_t val = _stackCPU_popByte(cpu, mem, CPU_ESTACK_ENABLE);
    if (cpu->P.E)
    {
        _cpu_set_sr(cpu, (sr & 0x20) | (val & 0xdf)); // Bit 5 is unaffected by operation in emulation mode
    }
    else
    {
        _cpu_set_sr(cpu, val);
    }
    cpu->cycles += 4;

    _cpu_update_pc(cpu, 1);
}

void i_plx(CPU_t *cpu, memory_t *mem)
{
    if (cpu->P.E)
    {
        cpu->X = _stackCPU_popByte(cpu, mem, CPU_ESTACK_ENABLE);
        cpu->cycles += 4;
        cpu->P.Z = ((cpu->X & 0xff) == 0);
        cpu->P.N = ((cpu->X & 0x80) == 0x80);
    }
    else
    {
        if (cpu->P.XB)
        {
            cpu->X = _stackCPU_popByte(cpu, mem, CPU_ESTACK_ENABLE);
            cpu->cycles += 4;
            cpu->P.Z = ((cpu->X & 0xff) == 0);
            cpu->P.N = ((cpu->X & 0x80) == 0x80);
        }
        else // 16-bit A
        {
            cpu->X = _stackCPU_popWord(cpu, mem, CPU_ESTACK_ENABLE);
            cpu->cycles += 5;
            cpu->P.Z = (cpu->X == 0);
            cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
        }
    }

    _cpu_update_pc(cpu, 1);
}

void i_ply(CPU_t *cpu, memory_t *mem)
{
    if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit X
    {
        cpu->Y = _stackCPU_popByte(cpu, mem, CPU_ESTACK_ENABLE);
        cpu->cycles += 4;
        cpu->P.Z = ((cpu->Y & 0xff) == 0);
        cpu->P.N = ((cpu->Y & 0x80) == 0x80);
    }
    else // 16-bit X
    {
        cpu->Y = _stackCPU_popWord(cpu, mem, CPU_ESTACK_ENABLE);
        cpu->cycles += 5;
        cpu->P.Z = (cpu->Y == 0);
        cpu->P.N = ((cpu->Y & 0x8000) == 0x8000);
    }

    _cpu_update_pc(cpu, 1);
}

void i_rep(CPU_t *cpu, memory_t *mem)
{
    uint8_t sr = _cpu_get_sr(cpu);
    uint8_t val = _cpu_get_immd_byte(cpu, mem);

    if (cpu->P.E)
    {
        _cpu_set_sr(cpu, sr & ((~val) | 0x30)); // Bits 4 and 5 are unaffected by operation in emulation mode
    }
    else
    {
        _cpu_set_sr(cpu, sr & (~val));

        if (cpu->P.XB) // Short X = zero top byte
        {
            cpu->X &= 0xff;
            cpu->Y &= 0xff;
        }
    }

    _cpu_update_pc(cpu, 2);
    cpu->cycles += 3;
}

void i_rol(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    uint16_t post_data = 0;
    uint16_t pre_data = 0;
    switch (mode)
    {
    case CPU_ADDR_DP:
    case CPU_ADDR_DPX:
        pre_data = _get_mem_word_bank_wrap(mem, addr);

        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            post_data = ((pre_data << 1) & 0xff) | cpu->P.C;
            _set_mem_byte(mem, addr, (uint8_t)post_data);
        }
        else // 16-bit
        {
            post_data = (pre_data << 1) | cpu->P.C;
            _set_mem_word_bank_wrap(mem, addr, post_data);
            cpu->cycles += 2;
        }

        // If DL != 0, add a cycle
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
        break;
    case CPU_ADDR_ABS:
    case CPU_ADDR_ABSX:
        pre_data = _get_mem_word(mem, addr);

        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            post_data = ((pre_data << 1) & 0xff) | cpu->P.C;
            _set_mem_byte(mem, addr, (uint8_t)post_data);
        }
        else // 16-bit
        {
            post_data = (pre_data << 1) | cpu->P.C;
            _set_mem_word(mem, addr, post_data);
            cpu->cycles += 2;
        }
        break;
    case CPU_ADDR_IMPD:
        pre_data = cpu->C;

        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            post_data = ((pre_data << 1) & 0xff) | cpu->P.C;
            cpu->C = (cpu->C & 0xff00) | post_data;
        }
        else // 16-bit
        {
            post_data = (pre_data << 1) | cpu->P.C;
            cpu->C = post_data;
        }
        break;
    }

    if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
    {
        cpu->C = (pre_data & 0x80) ? 1 : 0;
        cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
        cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
    }
    else // 16-bit
    {
        cpu->P.C = (pre_data & 0x8000) ? 1 : 0;
        cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
        cpu->P.Z = cpu->C ? 0 : 1;
    }

    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_ror(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    uint16_t post_data = 0;
    uint16_t pre_data = 0;
    switch (mode)
    {
    case CPU_ADDR_DP:
    case CPU_ADDR_DPX:
        pre_data = _get_mem_word_bank_wrap(mem, addr);

        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            post_data = ((pre_data >> 1) & 0xff) | (cpu->P.C << 7);
            _set_mem_byte(mem, addr, (uint8_t)post_data);
        }
        else // 16-bit
        {
            post_data = (pre_data >> 1) | (cpu->P.C << 15);
            _set_mem_word_bank_wrap(mem, addr, post_data);
            cpu->cycles += 2;
        }

        // If DL != 0, add a cycle
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
        break;
    case CPU_ADDR_ABS:
    case CPU_ADDR_ABSX:
        pre_data = _get_mem_word(mem, addr);

        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            post_data = ((pre_data >> 1) & 0xff) | (cpu->P.C << 7);
            _set_mem_byte(mem, addr, (uint8_t)post_data);
        }
        else // 16-bit
        {
            post_data = (pre_data >> 1) | (cpu->P.C << 15);
            _set_mem_word(mem, addr, post_data);
            cpu->cycles += 2;
        }
        break;
    case CPU_ADDR_IMPD:
        pre_data = cpu->C;

        if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
        {
            post_data = ((pre_data >> 1) & 0xff) | (cpu->P.C << 7);
            cpu->C = (cpu->C & 0xff00) | post_data;
        }
        else // 16-bit
        {
            post_data = (pre_data >> 1) | (cpu->P.C << 15);
            cpu->C = post_data;
        }
        break;
    }

    if (cpu->P.E || (!cpu->P.E && cpu->P.XB)) // 8-bit
    {
        cpu->C = (pre_data & 0x80) ? 1 : 0;
        cpu->P.N = (cpu->C & 0x80) ? 1 : 0;
        cpu->P.Z = (cpu->C & 0xff) ? 0 : 1;
    }
    else // 16-bit
    {
        cpu->P.C = (pre_data & 0x8000) ? 1 : 0;
        cpu->P.N = (cpu->C & 0x8000) ? 1 : 0;
        cpu->P.Z = cpu->C ? 0 : 1;
    }

    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_rti(CPU_t *cpu, memory_t *mem)
{
    uint8_t sr = _cpu_get_sr(cpu);
    uint8_t val = _stackCPU_popByte(cpu, mem, CPU_ESTACK_ENABLE);

    if (cpu->P.E)
    {
        _cpu_set_sr(cpu, (sr & 0x30) | (val & 0xcf)); // Bits 4 and 5 are unaffected by operation in emulation mode
        cpu->PC = _stackCPU_popWord(cpu, mem, CPU_ESTACK_ENABLE);
        cpu->cycles += 6;
    }
    else
    {
        _cpu_set_sr(cpu, val);
        uint32_t data = _stackCPU_pop24(cpu, mem);
        cpu->PBR = (data & 0xff0000) >> 16;
        cpu->PC = data & 0xffff;
        cpu->cycles += 7;
    }
}

void i_rtl(CPU_t *cpu, memory_t *mem)
{
    uint32_t addr = _stackCPU_pop24(cpu, mem);
    cpu->PC = _addr_add_val_bank_wrap(addr & 0xffff, 1);
    cpu->PBR = (addr >> 16) & 0xff;
    cpu->cycles += 6;
}

void i_rts(CPU_t *cpu, memory_t *mem)
{
    cpu->PC = _addr_add_val_bank_wrap(_stackCPU_popWord(cpu, mem, CPU_ESTACK_ENABLE), 1);
    cpu->cycles += 6;
}

void i_sec(CPU_t *cpu)
{
    cpu->P.C = 1;
    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_sed(CPU_t *cpu)
{
    cpu->P.D = 1;
    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_sei(CPU_t *cpu)
{
    cpu->P.I = 1;
    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}

void i_sep(CPU_t *cpu, memory_t *mem)
{
    uint8_t sr = _cpu_get_sr(cpu);
    uint8_t val = _get_mem_byte(mem, _addr_add_val_bank_wrap(cpu->PC, 1));

    if (cpu->P.E)
    {
        _cpu_set_sr(cpu, sr | (val & 0xcf)); // Bits 4 and 5 are unaffected by operation in emulation mode
    }
    else
    {
        _cpu_set_sr(cpu, sr | val);

        if (cpu->P.XB)
        {
            cpu->X &= 0xff;
            cpu->Y &= 0xff;
        }
    }

    _cpu_update_pc(cpu, 2);
    cpu->cycles += 3;
}

void i_stp(CPU_t *cpu)
{
    //_cpu_update_pc(cpu, 1); // ???
    cpu->cycles += 3;
    cpu->P.STP = 1;
}

void i_stx(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_DPY)
    {
        _set_mem_byte(mem, addr, cpu->X & 0xff);
        if (!cpu->P.E && !cpu->P.XB) // 16-bit
        {
            _set_mem_byte(mem, _addr_add_val_bank_wrap(addr, 1), (cpu->X >> 8) & 0xff); // Bank wrapping
            cpu->cycles += 1;
        }
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_ABS)
    {
        _set_mem_byte(mem, addr, cpu->X & 0xff);
        if (!cpu->P.E && !cpu->P.XB) // 16-bit
        {
            _set_mem_byte(mem, addr + 1, (cpu->X >> 8) & 0xff); // No bank wrapping
            cpu->cycles += 1;
        }
    }

    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_sty(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_DPX)
    {
        _set_mem_byte(mem, addr, cpu->Y & 0xff);
        if (!cpu->P.E && !cpu->P.XB) // 16-bit
        {
            _set_mem_byte(mem, _addr_add_val_bank_wrap(addr, 1), (cpu->Y >> 8) & 0xff); // Bank wrapping
            cpu->cycles += 1;
        }
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_ABS)
    {
        _set_mem_byte(mem, addr, cpu->Y & 0xff);
        if (!cpu->P.E && !cpu->P.XB) // 16-bit
        {
            _set_mem_byte(mem, addr + 1, (cpu->Y >> 8) & 0xff); // No bank wrapping
            cpu->cycles += 1;
        }
    }

    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_stz(CPU_t *cpu, memory_t *mem, uint8_t size, uint8_t cycles, CPU_Addr_Mode_t mode, uint32_t addr)
{
    if (mode == CPU_ADDR_DP || mode == CPU_ADDR_DPX)
    {
        _set_mem_byte(mem, addr, 0);
        if (!cpu->P.M) // 16-bit
        {
            _set_mem_byte(mem, _addr_add_val_bank_wrap(addr, 1), 0); // Bank wrapping
            cpu->cycles += 1;
        }
        if (cpu->D & 0xff)
        {
            cpu->cycles += 1;
        }
    }
    else if (mode == CPU_ADDR_ABS || mode == CPU_ADDR_ABSX)
    {
        _set_mem_byte(mem, addr, 0);
        if (!cpu->P.M) // 16-bit
        {
            _set_mem_byte(mem, addr + 1, 0); // No bank wrapping
            cpu->cycles += 1;
        }
    }

    cpu->cycles += cycles;
    _cpu_update_pc(cpu, size);
}

void i_wai(CPU_t *cpu)
{
    if (cpu->P.NMI || cpu->P.IRQ)
    {
        cpu->cycles += 3;
        _cpu_update_pc(cpu, 1);
        // Jump to NMI or IRQ handler will happen at end of the step() function
    }
}

void i_wdm(CPU_t *cpu)
{
    _cpu_update_pc(cpu, 2);
    cpu->cycles += 2; // http://www.6502.org/tutorials/65c816opcodes.html#6.7
}

void i_xba(CPU_t *cpu)
{
    cpu->C = ((cpu->C << 8) | ((cpu->C >> 8) & 0xff)) & 0xffff;
    cpu->P.N = cpu->C & 0x80 ? 1 : 0;
    cpu->P.Z = cpu->C & 0xff ? 0 : 1;
    _cpu_update_pc(cpu, 1);
    cpu->cycles += 3;
}

void i_xce(CPU_t *cpu)
{
    unsigned char temp = cpu->P.E;
    cpu->P.E = cpu->P.C;
    cpu->P.C = temp;

    if (cpu->P.E)
    {
        cpu->P.M = 1; // Brk
        cpu->X &= 0xff;
        cpu->Y &= 0xff;
        cpu->SP = (cpu->SP & 0xff) | 0x0100;
    }
    else
    {
        cpu->P.M = 1; // ??? when staying in native mode
        cpu->P.XB = 1; // ??? when staying in native mode
    }

    _cpu_update_pc(cpu, 1);
    cpu->cycles += 2;
}
