
#include <65816.h>

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
    cpu->Y |= 0x00ff;
    cpu->P.M = 1;
    cpu->P.X = 1;
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
CPU_Error_Code_t stepCPU(CPU_t *cpu, int16_t *mem)
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

    // Fetch, decode, execute instruction
    switch (mem[cpu->PC])
    {
        case 0x00: // BRK

            // Push PC and SR
            if (!cpu->P.E)
            {
                _stackCPU_pushByte(cpu, mem, cpu->PBR);
            }
            CPU_UPDATE_PC16(cpu, 2);
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
            CPU_UPDATE_PC16(cpu, 2);
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

        case 0x20: // JSR addr
            _stackCPU_pushWord(cpu, mem, ADDR_ADD_VAL_BANK_WRAP(cpu->PC, 2));
            cpu->PC = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
            cpu->cycles += 6;
            break;

        case 0x22: // JSL/JSR long
            // ??? Unknown operation in emulation mode. The stack may be treated differently.
            _stackCPU_pushByte(cpu, mem, cpu->PBR);
            _stackCPU_pushWord(cpu, mem, CPU_GET_EFFECTIVE_PC24(cpu) + 3);
            cpu->PBR = mem[ADDR_ADD_VAL_BANK_WRAP(CPU_GET_EFFECTIVE_PC24(cpu), 3)] & 0xff;
            cpu->PC = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
            cpu->cycles += 8;
            break;

        case 0x40: // RTI
            CPU_SET_SR(cpu, _stackCPU_popByte(cpu, mem));
            cpu->PC = _stackCPU_popWord(cpu, mem);
            cpu->cycles += 6;

            if (!cpu->P.E)
            {
                cpu->PBR = _stackCPU_popByte(cpu, mem);
                cpu->cycles += 1;
            }

            break;

        case 0x42: // WDM
            CPU_UPDATE_PC16(cpu, 2);
            cpu->cycles += 2; // ???
            break;

        case 0x4c: // JMP addr
            cpu->PC = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
            cpu->cycles += 3;
            break;

        case 0x5c: // JMP long
            cpu->PC = ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(CPU_GET_EFFECTIVE_PC24(cpu), 3));
            cpu->PC = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
            cpu->cycles += 4;
            break;

        case 0x60: // RTS
            cpu->PC = ADDR_ADD_VAL_BANK_WRAP(_stackCPU_popWord(cpu, mem), 1);
            cpu->PBR = _stackCPU_popByte(cpu, mem);
            cpu->cycles += 6;
            break;

        case 0x6b: // RTL
            // ??? Unknown operation in emulation mode. The stack may be treated differently.
            cpu->PC = ADDR_ADD_VAL_BANK_WRAP(_stackCPU_popWord(cpu, mem), 1);
            cpu->PBR = _stackCPU_popByte(cpu, mem);
            cpu->cycles += 6;
            break;

        case 0x6c: // JMP (addr)
            cpu->PC = _addrCPU_getAbsoluteIndirect(cpu, mem);
            cpu->cycles += 5;
            break;

        case 0x7c: // JMP (addr,X)
            cpu->PC = _addrCPU_getAbsoluteIndexedIndirectX(cpu, mem);
            cpu->cycles += 6;
            break;

        case 0xdc: // JMP [addr]
        {
            int32_t addr = _addrCPU_getAbsoluteIndirectLong(cpu, mem);
            cpu->PBR = (addr & 0xff0000) >> 16;
            cpu->PC = addr & 0xffff;
            cpu->cycles += 6;
        }
            break;

        case 0xea: // NOP
            CPU_UPDATE_PC16(cpu, 1);
            cpu->cycles += 2;
            break;

        case 0xeb: // XBA
            cpu->C = ( (cpu->C << 8) | ( (cpu->C >> 8) & 0xff ) ) & 0xffff;
            cpu->P.N = cpu->C & 0x80 ? 1 : 0;
            cpu->P.Z = cpu->C & 0xff ? 0 : 1;
            CPU_UPDATE_PC16(cpu, 1);
            cpu->cycles += 3;
            break;

        case 0xfc: // JSR (addr,X)
            _stackCPU_pushWord(cpu, mem, ADDR_ADD_VAL_BANK_WRAP(cpu->PC, 2));
            cpu->PC = _addrCPU_getAbsoluteIndexedIndirectX(cpu, mem);
            cpu->cycles += 8;
            break;

        default:
            return CPU_ERR_UNKNOWN_OPCODE;
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
        // cpu->P.I = 1; // IRQ flag is not set: https://softpixel.com/~cwright/sianse/docs/65816NFO.HTM#7.00

        return CPU_ERR_OK;
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

        return CPU_ERR_OK;
    }


    return CPU_ERR_OK;
}

/**
 * Push a byte (8-bits) onto the CPU's stack
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param byte The byte to be pushed onto the stack
 */
static void _stackCPU_pushByte(CPU_t *cpu, int16_t *mem, int32_t byte)
{
    mem[cpu->SP] = byte;

    // Correct SP if in emulation mode
    if ( cpu->P.E && ((cpu->SP & 0xff) == 0) )
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
static void _stackCPU_pushWord(CPU_t *cpu, int16_t *mem, int32_t word)
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
static int32_t _stackCPU_popByte(CPU_t *cpu, int16_t *mem)
{
    int32_t byte;

    byte = mem[cpu->SP] & 0xff;

    // Correct SP if in emulation mode
    if (cpu->P.E && ((cpu->SP & 0xff) == 0xff))
    {
        cpu->SP = 0x0100;
    }
    else
    {
        cpu->SP += 1;
    }

    return byte;
}

/**
 * Pop a word (16-bits) off the CPU's stack and return it
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @return The value popped off the stack
 */
static int32_t _stackCPU_popWord(CPU_t *cpu, int16_t *mem)
{
    int32_t word;
    word = _stackCPU_popByte(cpu, mem);
    word |= (_stackCPU_popByte(cpu, mem) & 0xff) << 8;
    return word;
}

/**
 * Returns the 16-bit word in memory stored at the (addr, X)
 * from the current instruction.(i.e. the PC part of the resultant
 * indirect addresss)
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the indirect address
 * @return The word in memory at the indirect address (in the current PRB bank)
 */
static int32_t _addrCPU_getAbsoluteIndexedIndirectX(CPU_t *cpu, int16_t *mem)
{
    // Get the immediate operand word of the current instruction
    int32_t address = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
    address += cpu->X;
    address &= 0xffff; // Wraparound
    address |= (cpu->PBR & 0xff) << 16;

    // Find and return the resultant indirect address value
    return ADDR_GET_MEM_BYTE(mem, address) | (ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(address, 1)) << 8);
}

/**
 * Returns the 16-bit word in memory stored at the (addr)
 * from the current instruction. (i.e. the PC part of the resultant
 * indirect addresss)
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the indirect address
 * @return The word in memory at the indirect address (in bank 0)
 */
static int32_t _addrCPU_getAbsoluteIndirect(CPU_t *cpu, int16_t *mem)
{
    // Get the immediate operand word of the current instruction
    // (from Bank 0)
    int32_t address = ADDR_GET_MEM_IMMD_WORD(cpu, mem);

    // Find and return the resultant indirect address value
    return ADDR_GET_MEM_BYTE(mem, address) | (ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(address, 1)) << 8);
}

/**
 * Returns the 24-bit word in memory stored at the [addr]
 * from the current instruction. (i.e. the PC part of the resultant
 * indirect addresss)
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the indirect address
 * @return The word and byte in memory at the indirect address (in bank 0)
 */
static int32_t _addrCPU_getAbsoluteIndirectLong(CPU_t *cpu, int16_t *mem)
{
    // Get the immediate operand word of the current instruction
    // (from Bank 0)
    int32_t address = ADDR_GET_MEM_IMMD_WORD(cpu, mem);

    // Find and return the resultant indirect address value
    return ADDR_GET_MEM_BYTE(mem, address) | (ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(address, 1)) << 8) |
           (ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(address, 2)) << 16);
}