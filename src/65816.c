
#include <65816.h>

/**
 * Resets a CPU to its post-/RST value
 * @param cpu The CPU to be reset
 */
void resetCPU(CPU *cpu)
{
    cpu->D = 0x0000;
    cpu->DBR = 0x00;
    cpu->PBR = 0x00;
    cpu->SP &= 0x00ff;
    cpu->SP |= 0x0100;
    cpu->X &= 0x00ff;
    cpu->Y |= 0x00ff;
    cpu->P.M = 1;
    cpu->P.X = 1;
    cpu->P.D = 0;
    cpu->P.I = 1;
    cpu->P.E = 1;

    // Internal use only, tell the sim that the CPU just reset
    cpu->P.RST = 1;
    cpu->cycles = 0;
}

/**
 * Steps a CPU by one mchine cycle
 * @param cpu The CPU to be stepped
 * @param mem The memory array which is to be connected to the CPU
 */
void stepCPU(CPU *cpu, int16_t *mem)
{
    // Handle CPU reset
    if (cpu->P.RST)
    {
        cpu->PC = mem[CPU_VEC_RESET];
        return;
    }

    // Fetch, decode, execute instruction
    switch (mem[cpu->PC])
    {
        case 0x00: // BRK

            // Push PC and SR
            if (!cpu->P.E)
            {
                _stackCPU_pushByte(cpu, mem, cpu->PBR);
            }
            cpu->PC += 2;
            _stackCPU_pushWord(cpu, mem, cpu->PC);

            if (cpu->P.E)
            {
                _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu) | 0x10); // B gets set on the stack
            }
            else
            {
                _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu));
            }

            // Jump to ISR
            if (cpu->P.E)
            {
                cpu->PC = mem[CPU_VEC_EMU_IRQ];
                cpu->PC |= mem[CPU_VEC_EMU_IRQ + 1] << 8;
                cpu->cycles += 7;
            }
            else
            {
                cpu->PC = mem[CPU_VEC_NATIVE_BRK];
                cpu->PC |= mem[CPU_VEC_NATIVE_BRK + 1] << 8;
                cpu->PBR = 0;
                cpu->cycles += 8;
            }

            cpu->P.D = 0; // Binary mode (65C02)
            cpu->P.I = 1;
            break;

        case 0x02: // COP

            // Push PC and SR
            if (!cpu->P.E)
            {
                _stackCPU_pushByte(cpu, mem, cpu->PBR);
            }
            cpu->PC += 2;
            _stackCPU_pushWord(cpu, mem, cpu->PC);

            if (cpu->P.E)
            {
                _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu) & 0xef); // ??? Unknown: the state of the B flag in ISR for COP (assumed to be 0)
            }
            else
            {
                _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu));
            }

            // Jump to ISR
            if (cpu->P.E)
            {
                cpu->PC = mem[CPU_VEC_EMU_COP];
                cpu->PC |= mem[CPU_VEC_EMU_COP + 1] << 8;
                cpu->cycles += 7;
            }
            else
            {
                cpu->PC = mem[CPU_VEC_NATIVE_COP];
                cpu->PC |= mem[CPU_VEC_NATIVE_COP + 1] << 8;
                cpu->PBR = 0;
                cpu->cycles += 8;
            }

            cpu->P.D = 0; // Binary mode (65C02)
            cpu->P.I = 1;
            break;
        case 0x42: // WDM
            cpu->PC += 2;
            cpu->cycles += 2; //????
            break;
        case 0xea: // NOP
            cpu->PC += 1;
            cpu->cycles += 2;
            break;
        case 0xeb: // XBA
            cpu->C = ( (cpu->C << 8) | ( (cpu->C >> 8) & 0xff ) ) & 0xffff;
            cpu->P.N = cpu->C & 0x80 ? 1 : 0;
            cpu->P.Z = cpu->C & 0xff ? 0 : 1;
            cpu->PC += 1;
            cpu->cycles += 3;
            break;
        default:
            printf("Unknown opcode: %d\n", mem[cpu->PC]);
            break;
    }


    // Handle any interrupts that are pending
    if (cpu->P.NMI)
    {
        cpu->P.NMI = 0;

        // Push PC and SR
        if (!cpu->P.E)
        {
            _stackCPU_pushByte(cpu, mem, cpu->PBR);
        }
        _stackCPU_pushWord(cpu, mem, cpu->PC);

        if (cpu->P.E)
        {
            _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu) & 0xef); // B gets reset on the stack
        }
        else
        {
            _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu));
        }

        // Jump to ISR
        if (cpu->P.E)
        {
            cpu->PC = mem[CPU_VEC_EMU_NMI];
            cpu->PC |= mem[CPU_VEC_EMU_NMI + 1] << 8;
            cpu->cycles += 7;
        }
        else
        {
            cpu->PC = mem[CPU_VEC_NATIVE_NMI];
            cpu->PC |= mem[CPU_VEC_NATIVE_NMI + 1] << 8;
            cpu->PBR = 0;
            cpu->cycles += 8;
        }

        cpu->P.D = 0; // Binary mode (65C02)
        cpu->P.I = 1;

        return;
    }
    if (cpu->P.IRQ && !cpu->P.I)
    {
        cpu->P.IRQ = 0;

        // Push PC and SR
        if (!cpu->P.E)
        {
            _stackCPU_pushByte(cpu, mem, cpu->PBR);
        }
        _stackCPU_pushWord(cpu, mem, cpu->PC);

        if (cpu->P.E)
        {
            _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu) & 0xef); // B gets reset on the stack
        }
        else
        {
            _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu));
        }
        // Jump to ISR
        if (cpu->P.E)
        {
            cpu->PC = mem[CPU_VEC_EMU_IRQ];
            cpu->PC |= mem[CPU_VEC_EMU_IRQ + 1] << 8; 
            cpu->cycles += 7;
        }
        else
        {
            cpu->PC = mem[CPU_VEC_NATIVE_IRQ];
            cpu->PC |= mem[CPU_VEC_NATIVE_IRQ + 1] << 8;
            cpu->PBR = 0;
            cpu->cycles += 8;
        }

        cpu->P.D = 0; // Binary mode (65C02)
        cpu->P.I = 1;

        return;
    }
}

/**
 * Push a byte (8-bits) onto the CPU's stack
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param byte The byte to be pushed onto the stack
 */
static void _stackCPU_pushByte(CPU *cpu, int16_t *mem, int32_t byte)
{
    mem[cpu->SP] = byte;

    // Correct SP if in emulation mode
    if ( cpu->P.E && (cpu->SP & 0xff == 0) )
    {
        cpu->SP = 0x01ff;
    }
    else
    {
        cpu->SP -= 1;
    }
}

/**
 * Push a word (16-bits) onto the CPU's stack
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param word The word to be pushed onto the stack
 */
static void _stackCPU_pushWord(CPU *cpu, int16_t *mem, int32_t word)
{
    _stackCPU_pushByte(cpu, mem, (word & 0xff00) >> 8);
    _stackCPU_pushByte(cpu, mem, word & 0xff);
}

/**
 * Pop a byte (8-bits) off the CPU's stack and return it
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @return The value popped off the stack
 */
static int32_t _stackCPU_popByte(CPU *cpu, int16_t *mem)
{

}

/**
 * Pop a word (16-bits) off the CPU's stack and return it
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @return The value popped off the stack
 */
static int32_t _stackCPU_popWord(CPU *cpu, int16_t *mem)
{

}

/**
 * Decrement a reg's value depending on if the CPU is in
static int32_t regCPU_dec(CPU *cpu, int32_t val)
{
    if (cpu->P.E)
    {
        uint8_t temp = cpu->SP;
        temp -= 1;
        val &= ~0xff;
        val |= temp;
    }
    else
    {
        val -= 1;
    }
    return val;
}*/