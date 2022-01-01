
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
    // Somehow need to insert a jump to the vector into the IR
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


    // Handle any interrupts that are pending
    if (cpu->P.NMI)
    {
        return;
    }
    if (cpu->P.IRQ)
    {
        return;
    }
}

/**
 * Push a byte (8-bits) onto the CPU's stack
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param byte The byte to be pushed onto the stack
 */
static void stackCPU_pushByte(CPU *cpu, int16_t *mem, int32_t byte)
{
    mem[cpu->SP--] = byte;
}

/**
 * Push a word (16-bits) onto the CPU's stack
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param word The word to be pushed onto the stack
 */
static void stackCPU_pushWord(CPU *cpu, int16_t *mem, int32_t word)
{
    mem[cpu->SP--] = (word & 0xff00) >> 8;
    mem[cpu->SP--] = word & 0xff;
}

/**
 * Pop a byte (8-bits) off the CPU's stack and return it
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @return The value popped off the stack
 */
static int32_t stackCPU_popByte(CPU *cpu, int16_t *mem)
{

}

/**
 * Pop a word (16-bits) off the CPU's stack and return it
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @return The value popped off the stack
 */
static int32_t stackCPU_popWord(CPU *cpu, int16_t *mem)
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