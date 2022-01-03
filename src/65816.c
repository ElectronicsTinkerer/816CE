
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
CPU_Error_Code_t stepCPU(CPU_t *cpu, uint8_t *mem)
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
        case 0x00: // BRK

            CPU_UPDATE_PC16(cpu, 2);

            if (cpu->P.E)
            {
                _stackCPU_pushWord(cpu, mem, cpu->PC, CPU_ESTACK_ENABLE);
                _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu) | 0x10); // B flag is set for BRK in emulation mode
                cpu->PC = mem[CPU_VEC_EMU_IRQ];
                cpu->PC |= mem[CPU_VEC_EMU_IRQ + 1] << 8;
                cpu->PBR = 0;
                cpu->cycles += 7;
            }
            else
            {
                _stackCPU_push24(cpu, mem, CPU_GET_EFFECTIVE_PC(cpu));
                _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu));
                cpu->PC = mem[CPU_VEC_NATIVE_BRK];
                cpu->PC |= mem[CPU_VEC_NATIVE_BRK + 1] << 8;
                cpu->PBR = 0;
                cpu->cycles += 8;
            }

            cpu->P.D = 0; // Binary mode (65C02)
            cpu->P.I = 1;
            break;

        case 0x02: // COP

            CPU_UPDATE_PC16(cpu, 2);

            if (cpu->P.E)
            {
                _stackCPU_pushWord(cpu, mem, cpu->PC, CPU_ESTACK_ENABLE);
                _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu) & 0xef); // ??? Unknown: the state of the B flag in ISR for COP (assumed to be 0)
                cpu->PC = mem[CPU_VEC_EMU_COP];
                cpu->PC |= mem[CPU_VEC_EMU_COP + 1] << 8;
                cpu->PBR = 0;
                cpu->cycles += 7;
            }
            else
            {
                _stackCPU_push24(cpu, mem, CPU_GET_EFFECTIVE_PC(cpu));
                _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu));
                cpu->PC = mem[CPU_VEC_NATIVE_COP];
                cpu->PC |= mem[CPU_VEC_NATIVE_COP + 1] << 8;
                cpu->PBR = 0;
                cpu->cycles += 8;
            }

            cpu->P.D = 0; // Binary mode (65C02)
            cpu->P.I = 1;
            break;

        case 0x08: // PHP
            _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu));
            cpu->cycles += 3;
            CPU_UPDATE_PC16(cpu, 1);
            break;

        case 0x0b: // PHD
            _stackCPU_pushWord(cpu, mem, cpu->D, CPU_ESTACK_DISABLE);
            cpu->cycles += 4;
            CPU_UPDATE_PC16(cpu, 1);
            break;

        case 0x18: // CLC
            cpu->P.C = 0;
            CPU_UPDATE_PC16(cpu, 1);
            cpu->cycles += 2;
            break;

        case 0x20: // JSR addr
            _stackCPU_pushWord(cpu, mem, ADDR_ADD_VAL_BANK_WRAP(cpu->PC, 2), CPU_ESTACK_ENABLE);
            cpu->PC = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
            cpu->cycles += 6;
            break;

        case 0x22: // JSL/JSR long
            _stackCPU_push24(cpu, mem, ADDR_ADD_VAL_BANK_WRAP(CPU_GET_EFFECTIVE_PC(cpu), 3));
            cpu->PBR = mem[ADDR_ADD_VAL_BANK_WRAP(CPU_GET_EFFECTIVE_PC(cpu), 3)] & 0xff;
            cpu->PC = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
            cpu->cycles += 8;
            break;

        case 0x28: // PLP
        {
            uint8_t sr = CPU_GET_SR(cpu);
            uint8_t val = _stackCPU_popByte(cpu, mem, CPU_ESTACK_ENABLE);
            if (cpu->P.E)
            {
                CPU_SET_SR(cpu, (sr & 0x20) | (val & 0xdf)); // Bit 5 is unaffected by operation in emulation mode
            }
            else
            {
                CPU_SET_SR(cpu, val);
            }
        }
            cpu->cycles += 4;

            CPU_UPDATE_PC16(cpu, 1);

            break;

        case 0x2b: // PLD
            cpu->D = _stackCPU_popWord(cpu, mem, CPU_ESTACK_DISABLE);
            cpu->cycles += 5;
            cpu->P.Z = ((cpu->DBR & 0xffff) == 0);
            cpu->P.N = ((cpu->DBR & 0x8000) == 0x8000);

            CPU_UPDATE_PC16(cpu, 1);

            break;

        case 0x38: // SEC
            cpu->P.C = 1;
            CPU_UPDATE_PC16(cpu, 1);
            cpu->cycles += 2;
            break;

        case 0x40: // RTI
        {
            uint8_t sr = CPU_GET_SR(cpu);
            uint8_t val = _stackCPU_popByte(cpu, mem, CPU_ESTACK_ENABLE);

            if (cpu->P.E)
            {
                CPU_SET_SR(cpu, (sr & 0x30) | (val & 0xcf)); // Bits 4 and 5 are unaffected by operation in emulation mode
                cpu->PC = _stackCPU_popWord(cpu, mem, CPU_ESTACK_ENABLE);
                cpu->cycles += 6;
            }
            else
            {
                CPU_SET_SR(cpu, val);
                int32_t data = _stackCPU_pop24(cpu, mem);
                cpu->PBR = (data & 0xff0000) >> 16;
                cpu->PC = data & 0xffff;
                cpu->cycles += 7;
            }
        }
            break;

        case 0x42: // WDM
            CPU_UPDATE_PC16(cpu, 2);
            cpu->cycles += 2; // ???
            break;

        case 0x48: // PHA
            if (cpu->P.E)
            {
                _stackCPU_pushByte(cpu, mem, cpu->C);
                cpu->cycles += 3;
            }
            else
            {
                if (cpu->P.M) // 8-bit A
                {
                    _stackCPU_pushByte(cpu, mem, cpu->C);
                    cpu->cycles += 3;
                }
                else // 16-bit A
                {
                    _stackCPU_pushWord(cpu, mem, cpu->C, CPU_ESTACK_ENABLE);
                    cpu->cycles += 4;
                }
            }

            CPU_UPDATE_PC16(cpu, 1);

            break;

        case 0x4b: // PHK
            _stackCPU_pushByte(cpu, mem, cpu->PBR);
            cpu->cycles += 3;
            CPU_UPDATE_PC16(cpu, 1);
            break;

        case 0x4c: // JMP addr
            cpu->PC = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
            cpu->cycles += 3;
            break;

        case 0x58: // CLI
            cpu->P.I = 0;
            CPU_UPDATE_PC16(cpu, 1);
            cpu->cycles += 2;
            break;

        case 0x5a: // PHY
            if (cpu->P.E)
            {
                _stackCPU_pushByte(cpu, mem, cpu->Y);
                cpu->cycles += 3;
            }
            else
            {
                if (cpu->P.X) // 8-bit X
                {
                    _stackCPU_pushByte(cpu, mem, cpu->Y);
                    cpu->cycles += 3;
                }
                else // 16-bit X
                {
                    _stackCPU_pushWord(cpu, mem, cpu->Y, CPU_ESTACK_ENABLE);
                    cpu->cycles += 4;
                }
            }

            CPU_UPDATE_PC16(cpu, 1);

            break;

        case 0x5c: // JMP long
            cpu->PC = ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(CPU_GET_EFFECTIVE_PC(cpu), 3));
            cpu->PC = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
            cpu->cycles += 4;
            break;

        case 0x60: // RTS
            cpu->PC = ADDR_ADD_VAL_BANK_WRAP(_stackCPU_popWord(cpu, mem, CPU_ESTACK_ENABLE), 1);
            cpu->cycles += 6;
            break;

        case 0x62: // PER label
        {
            int16_t displacement = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
            CPU_UPDATE_PC16(cpu, 3);
            _stackCPU_pushWord(cpu, mem, (cpu->PC + displacement) & 0xffff, CPU_ESTACK_DISABLE);

            cpu->cycles += 6;
        }
            break;

        case 0x68: // PLA
            if (cpu->P.E || cpu->P.M)
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
                cpu->P.Z = ((cpu->C & 0xffff) == 0);
                cpu->P.N = ((cpu->C & 0x8000) == 0x8000);
            }

            CPU_UPDATE_PC16(cpu, 1);

            break;

        case 0x6b: // RTL
        {
            int32_t addr = _stackCPU_pop24(cpu, mem);
            cpu->PC = ADDR_ADD_VAL_BANK_WRAP(addr & 0xffff, 1);
            cpu->PBR = (addr & 0xff0000) >> 16;
            cpu->cycles += 6;
        }
            break;

        case 0x6c: // JMP (addr)
            cpu->PC = _addrCPU_getAbsoluteIndirect(cpu, mem);
            cpu->cycles += 5;
            break;

        case 0x78: // SEI
            cpu->P.I = 1;
            CPU_UPDATE_PC16(cpu, 1);
            cpu->cycles += 2;
            break;

        case 0x7a: // PLY
            if (cpu->P.E || cpu->P.X)
            {
                cpu->Y = _stackCPU_popByte(cpu, mem, CPU_ESTACK_ENABLE);
                cpu->cycles += 4;
                cpu->P.Z = ((cpu->Y & 0xff) == 0);
                cpu->P.N = ((cpu->Y & 0x80) == 0x80);
            }
            else // 16-bit A
            {
                cpu->Y = _stackCPU_popWord(cpu, mem, CPU_ESTACK_ENABLE);
                cpu->cycles += 5;
                cpu->P.Z = ((cpu->Y & 0xffff) == 0);
                cpu->P.N = ((cpu->Y & 0x8000) == 0x8000);
            }

            CPU_UPDATE_PC16(cpu, 1);

            break;

        case 0x7c: // JMP (addr,X)
            cpu->PC = _addrCPU_getAbsoluteIndexedIndirectX(cpu, mem);
            cpu->cycles += 6;
            break;

        case 0x8b: // PHB
            _stackCPU_pushByte(cpu, mem, cpu->DBR);
            cpu->cycles += 3;
            CPU_UPDATE_PC16(cpu, 1);
            break;

        case 0xab: // PLB
            cpu->DBR = _stackCPU_popByte(cpu, mem, CPU_ESTACK_DISABLE);
            cpu->cycles += 4;
            cpu->P.Z = ((cpu->DBR & 0xff) == 0);
            cpu->P.N = ((cpu->DBR & 0x80) == 0x80);

            CPU_UPDATE_PC16(cpu, 1);

            break;

        case 0xb8: // CLV
            cpu->P.V = 0;
            CPU_UPDATE_PC16(cpu, 1);
            cpu->cycles += 2;
            break;

        case 0xc2: // REP
        {
            uint8_t sr = CPU_GET_SR(cpu);
            uint8_t val = ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(cpu->PC, 1));

            if (cpu->P.E)
            {
                CPU_SET_SR(cpu, sr & ((~val) | 0x30)); // Bits 4 and 5 are unaffected by operation in emulation mode
            }
            else
            {
                CPU_SET_SR(cpu, sr & (~val));
            }

            CPU_UPDATE_PC16(cpu, 2);
            cpu->cycles += 3;
        }
            break;

        case 0xd4: // PEI (dp)
        {
            int32_t addr_dp = ADDR_ADD_VAL_BANK_WRAP((cpu->D & 0xffff), ADDR_GET_MEM_IMMD_BYTE(cpu, mem));
            int32_t addr_ind = ADDR_GET_MEM_BYTE(mem, addr_dp);
            addr_ind |= ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(addr_dp, 1));
            _stackCPU_pushWord(cpu, mem, addr_ind, CPU_ESTACK_DISABLE);
            
            CPU_UPDATE_PC16(cpu, 2);
            cpu->cycles += 6;
            if (cpu->D & 0xff) // Add a cycle if DL != 0
            {
                cpu->cycles += 1;
            }
        }
            break;

        case 0xd8: // CLD
            cpu->P.D = 0;
            CPU_UPDATE_PC16(cpu, 1);
            cpu->cycles += 2;
            break;

        case 0xda: // PHX
            if (cpu->P.E)
            {
                _stackCPU_pushByte(cpu, mem, cpu->X);
                cpu->cycles += 3;
            }
            else
            {
                if (cpu->P.X) // 8-bit X
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

            CPU_UPDATE_PC16(cpu, 1);

            break;

        case 0xdb: // STP
            CPU_UPDATE_PC16(cpu, 1);
            cpu->cycles += 3;
            cpu->P.STP = 1;
            break;

        case 0xdc: // JMP [addr]
        {
            int32_t addr = _addrCPU_getAbsoluteIndirectLong(cpu, mem);
            cpu->PBR = (addr & 0xff0000) >> 16;
            cpu->PC = addr & 0xffff;
            cpu->cycles += 6;
        }
            break;

        case 0xe2: // SEP
        {
            uint8_t sr = CPU_GET_SR(cpu);
            uint8_t val = ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(cpu->PC, 1));

            if (cpu->P.E)
            {
                CPU_SET_SR(cpu, sr | (val & 0xcf)); // Bits 4 and 5 are unaffected by operation in emulation mode
            }
            else
            {
                CPU_SET_SR(cpu, sr | val);
            }

            CPU_UPDATE_PC16(cpu, 2);
            cpu->cycles += 3;
        }
            break;

        case 0xea: // NOP
            CPU_UPDATE_PC16(cpu, 1);
            cpu->cycles += 2;
            break;

        case 0xeb: // XBA
            cpu->C = ((cpu->C << 8) | ((cpu->C >> 8) & 0xff)) & 0xffff;
            cpu->P.N = cpu->C & 0x80 ? 1 : 0;
            cpu->P.Z = cpu->C & 0xff ? 0 : 1;
            CPU_UPDATE_PC16(cpu, 1);
            cpu->cycles += 3;
            break;

        case 0xf4: // PEA addr
            _stackCPU_pushWord(cpu, mem, ADDR_GET_MEM_IMMD_WORD(cpu, mem), CPU_ESTACK_DISABLE);
            cpu->cycles += 5;
            CPU_UPDATE_PC16(cpu, 3);
            break;

        case 0xf8: // SED
            cpu->P.D = 1;
            CPU_UPDATE_PC16(cpu, 1);
            cpu->cycles += 2;
            break;

        case 0xfa: // PLX
            if (cpu->P.E || cpu->P.X)
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
                cpu->P.Z = ((cpu->X & 0xffff) == 0);
                cpu->P.N = ((cpu->X & 0x8000) == 0x8000);
            }

            CPU_UPDATE_PC16(cpu, 1);

            break;

        case 0xfc: // JSR (addr,X)
            _stackCPU_pushWord(cpu, mem, ADDR_ADD_VAL_BANK_WRAP(cpu->PC, 2), CPU_ESTACK_DISABLE);
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

        if (cpu->P.E)
        {
            _stackCPU_pushWord(cpu, mem, cpu->PC, CPU_ESTACK_ENABLE);
            _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu) & 0xef); // B gets reset on the stack
            cpu->PC = mem[CPU_VEC_EMU_NMI];
            cpu->PC |= mem[CPU_VEC_EMU_NMI + 1] << 8;
            cpu->PBR = 0;
            cpu->cycles += 7;
        }
        else
        {
            _stackCPU_push24(cpu, mem, CPU_GET_EFFECTIVE_PC(cpu));
            _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu));
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
            _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu) & 0xef); // B gets reset on the stack
            cpu->PC = mem[CPU_VEC_EMU_IRQ];
            cpu->PC |= mem[CPU_VEC_EMU_IRQ + 1] << 8;
            cpu->PBR = 0;
            cpu->cycles += 7;
        }
        else
        {
            _stackCPU_push24(cpu, mem, CPU_GET_EFFECTIVE_PC(cpu));
            _stackCPU_pushByte(cpu, mem, CPU_GET_SR(cpu));
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
static void _stackCPU_pushByte(CPU_t *cpu, uint8_t *mem, int32_t byte)
{
    if (cpu->P.E)
    {
        mem[(cpu->SP & 0xff) | 0x0100] = byte;
        cpu->SP = ((cpu->SP - 1) & 0xff) | 0x0100;
    }
    else
    {
        mem[cpu->SP & 0xffff] = byte;
        cpu->SP -= 1;
        cpu->SP &= 0xffff;
    }
}

/**
 * Push a word (16-bits) onto the CPU's stack
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param word The word to be pushed onto the stack
 * @param emulationStack 1 if the stack should be limited to page 1,
 *                       0 for new instructions/native mode
 */
static void _stackCPU_pushWord(CPU_t *cpu, uint8_t *mem, int32_t word, Emul_Stack_Mod_t emulationStack)
{
    if ( cpu->P.E && emulationStack )
    {
        mem[(cpu->SP & 0xff) | 0x0100] = (word & 0xff00) >> 8;
        cpu->SP -= 1;
        mem[(cpu->SP & 0xff) | 0x0100] = word & 0xff;
        cpu->SP -= 1;
    }
    else
    {
        mem[cpu->SP & 0xffff] = (word & 0xff00) >> 8;
        cpu->SP -= 1;
        mem[cpu->SP & 0xffff] = word & 0xff;
        cpu->SP -= 1;
        cpu->SP &= 0xffff;
    }
    if (cpu->P.E)
    {
        cpu->SP &= 0xff;
        cpu->SP |= 0x0100;
    }
}

/**
 * Push a three bytes (24-bits) onto the CPU's stack
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param data The word to be pushed onto the stack
 */
static void _stackCPU_push24(CPU_t *cpu, uint8_t *mem, int32_t data)
{

    mem[cpu->SP & 0xffff] = (data & 0xff0000) >> 16;
    cpu->SP -= 1;
    mem[cpu->SP & 0xffff] = (data & 0xff00) >> 8;
    cpu->SP -= 1;
    mem[cpu->SP & 0xffff] = data & 0xff;
    cpu->SP -= 1;
    cpu->SP &= 0xffff;

    if (cpu->P.E)
    {
        cpu->SP &= 0xff;
        cpu->SP |= 0x0100;
    }
}

/**
 * Pop a byte (8-bits) off the CPU's stack and return it
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param emulationStack 1 if the stack should be limited to page 1,
 *                       0 for new instructions/native mode
 * @return The value popped off the stack
 */
static int32_t _stackCPU_popByte(CPU_t *cpu, uint8_t *mem, Emul_Stack_Mod_t emulationStack)
{
    int32_t byte = 0;
    if ( cpu->P.E && emulationStack )
    {
        cpu->SP = ( (cpu->SP + 1) & 0xff) | 0x0100;
        byte = ADDR_GET_MEM_BYTE(mem, cpu->SP);
    }
    else
    {
        cpu->SP += 1;
        cpu->SP &= 0xffff;
        byte = ADDR_GET_MEM_BYTE(mem, cpu->SP);
    }

    if (cpu->P.E)
    {
        cpu->SP &= 0xff;
        cpu->SP |= 0x0100;
    }

    return byte;
}

/**
 * Pop a word (16-bits) off the CPU's stack and return it
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param emulationStack 1 if the stack should be limited to page 1,
 *                       0 for new instructions/native mode
 * @return The value popped off the stack
 */
static int32_t _stackCPU_popWord(CPU_t *cpu, uint8_t *mem, Emul_Stack_Mod_t emulationStack)
{
    int32_t word = 0;
    if ( cpu->P.E && emulationStack )
    {
        cpu->SP = ((cpu->SP + 1) & 0xff) | 0x0100;
        word = ADDR_GET_MEM_BYTE(mem, cpu->SP); 
        cpu->SP = ((cpu->SP + 1) & 0xff) | 0x0100;
        word |= ADDR_GET_MEM_BYTE(mem, cpu->SP) << 8;
    }
    else
    {
        cpu->SP += 1;
        cpu->SP &= 0xffff;
        word = ADDR_GET_MEM_BYTE(mem, cpu->SP);
        cpu->SP += 1;
        cpu->SP &= 0xffff;
        word |= ADDR_GET_MEM_BYTE(mem, cpu->SP) << 8;
    }

    if (cpu->P.E)
    {
        cpu->SP &= 0xff;
        cpu->SP |= 0x0100;
    }

    return word;
}

/**
 * Pop a triple byte (24-bits) off the CPU's stack and return it
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @return The value popped off the stack
 */
static int32_t _stackCPU_pop24(CPU_t *cpu, uint8_t *mem)
{
    int32_t data = 0;

    cpu->SP += 1;
    cpu->SP &= 0xffff;
    data = ADDR_GET_MEM_BYTE(mem, cpu->SP);
    cpu->SP += 1;
    cpu->SP &= 0xffff;
    data |= ADDR_GET_MEM_BYTE(mem, cpu->SP) << 8;
    cpu->SP += 1;
    cpu->SP &= 0xffff;
    data |= ADDR_GET_MEM_BYTE(mem, cpu->SP) << 16;

    if (cpu->P.E)
    {
        cpu->SP &= 0xff;
        cpu->SP |= 0x0100;
    }

    return data;
}

/**
 * Returns the 16-bit word in memory stored at the (addr, X)
 * from the current instruction.(i.e. the PC part of the resultant
 * indirect addresss)
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the indirect address
 * @return The word in memory at the indirect address (in the current PRB bank)
 */
static int32_t _addrCPU_getAbsoluteIndexedIndirectX(CPU_t *cpu, uint8_t *mem)
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
static int32_t _addrCPU_getAbsoluteIndirect(CPU_t *cpu, uint8_t *mem)
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
static int32_t _addrCPU_getAbsoluteIndirectLong(CPU_t *cpu, uint8_t *mem)
{
    // Get the immediate operand word of the current instruction
    // (from Bank 0)
    int32_t address = ADDR_GET_MEM_IMMD_WORD(cpu, mem);

    // Find and return the resultant indirect address value
    return ADDR_GET_MEM_BYTE(mem, address) | (ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(address, 1)) << 8) |
           (ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(address, 2)) << 16);
}