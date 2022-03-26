
#include "65816-ops.h"

void i_brk(CPU_t *cpu, memory_t *mem)
{
    _cpu_update_pc(cpu, 2);

    if (cpu->P.E)
    {
        _stackCPU_pushWord(cpu, mem, cpu->PC, CPU_ESTACK_ENABLE);
        _stackCPU_pushByte(cpu, mem, _cpu_get_sr(cpu) | 0x10); // B flag is set for BRK in emulation mode
        cpu->PC = _get_mem_byte(cpu, CPU_VEC_EMU_IRQ);
        cpu->PC |= _get_mem_byte(cpu, CPU_VEC_EMU_IRQ + 1) << 8;
        cpu->PBR = 0;
        cpu->cycles += 7;
    }
    else
    {
        _stackCPU_push24(cpu, mem, _cpu_get_effective_pc(cpu));
        _stackCPU_pushByte(cpu, mem, _cpu_get_sr(cpu));
        cpu->PC = _get_mem_byte(cpu, CPU_VEC_NATIVE_BRK);
        cpu->PC |= _get_mem_byte(cpu, CPU_VEC_NATIVE_BRK + 1) << 8;
        cpu->PBR = 0;
        cpu->cycles += 8;
    }

    cpu->P.D = 0; // Binary mode (65C02)
    cpu->P.I = 1;
}

void i_pld(CPU_t *cpu, memory_t *mem)
{
    cpu->D = _stackCPU_popWord(cpu, mem, CPU_ESTACK_DISABLE);
    cpu->cycles += 5;
    cpu->P.Z = (cpu->D == 0);
    cpu->P.N = ((cpu->D & 0x8000) == 0x8000);

    _cpu_update_pc(cpu, 1);
}