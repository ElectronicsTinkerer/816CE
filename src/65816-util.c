
#include "65816-util.h"

/**
 * Add a value to the given CPU's PC (Bank wraps)
 * @param cpu The CPU to have its PC updated
 * @param offset The amount to add to the PC
 */
void _cpu_update_pc(CPU_t *cpu, uint16_t offset)
{
    cpu->PC += offset;
}

/**
 * Get the value of a CPU's SR
 * @param cpu A pointer to the CPU struct to get the SR from
 * @return The 8-bit value of the given CPU's SR
 */
uint8_t _cpu_get_sr(CPU_t *cpu)
{
    return *(uint8_t *) &(cpu->P);
}

/**
 * Set the value of the CPU's SR
 * @param cpu A pointer to the CPU struct which is to have its SR modified
 * @param sr The 8-bit value to load into the CPU SR
 */
void _cpu_set_sr(CPU_t *cpu, uint8_t sr)
{
    *(uint8_t *) &(cpu->P) = sr;
}

/**
 * Set the value of the CPU's SP
 * @note This enforces emulation mode page restrictions on the address
 * @param cpu The CPU to have its SP modified
 * @param addr The value of the SP to set
 */
void _cpu_set_sp(CPU_t *cpu, uint16_t addr)
{
    if (cpu->P.E)
    {
        cpu->SP = (addr & 0xff) | 0x0100;
    }
    else
    {
        cpu->SP = addr;
    }
   
}

/**
 * Get the CPU's PROGRAM BANK, shifted to be bits 16..23 of the value
 * @param cpu A pointer to the CPU struct from which the PBR will be retrieved
 * @return The PBR of the given cpu, placed in bits 23..16
 */
uint32_t _cpu_get_pbr(CPU_t * cpu)
{
    return (uint32_t)cpu->PBR << 16;
}

/**
 * Get a CPU's 24 bit PC address
 * @param cpu A pointer to the CPU struct from which to retrieve the 24-bit PC address
 * @return The cpu's PC concatenated with the PBR
 */
uint32_t _cpu_get_effective_pc(CPU_t *)
{
    return ((uint32_t)cpu->PBR << 16) | ((uint32_t)cpu->PC);
}

/**
 * Get the byte in memory at the address CPU PC+1
 * @note This will BANK WRAP
 * @param cpu The CPU from which to retrieve the PC
 * @param mem The memory from which to pull the value
 * @return The value of the byte in memory at the CPU's PC+1
 */
uint8_t _cpu_get_immd_byte(CPU_t *cpu, memory_t *mem)
{
    uint32_t addr = cpu->PC;
    addr += 1;
    addr &= 0xffff; // Bank wrap
    addr |= _cpu_get_pbr(cpu);
    return _get_mem_byte(mem, addr);
}

/**
 * Get the word in memory at the address CPU PC+1 (high byte @ PC+2)
 * @note This will BANK WRAP
 * @param cpu The CPU from which to retrieve the PC
 * @param mem The memory from which to pull the value
 * @return The value of the word in memory at the CPU's PC+1
 */
uint16_t _cpu_get_immd_word(CPU_t *cpu, memory_t *mem)
{
    uint32_t addr = cpu->PC;
    addr += 1;
    addr &= 0xffff; // Bank wrap
    addr |= _cpu_get_pbr(cpu);
    uint16_t val = _get_mem_byte(mem, addr);
    addr += 1;
    addr &= 0xffff; // Bank wrap
    addr |= _cpu_get_pbr(cpu);
    return val | (_get_mem_byte(mem, addr) << 8);
}

/**
 * Add a value to an address, PAGE WRAPPING
 * @param addr The base address of the operation
 * @param offset The amount to add to the base address
 * @return The addr plus offset, page wrapped
 */
uint32_t _addr_add_val_page_wrap(uint32_t addr, uint32_t offset)
{
    return (addr & 0x00ffff00) | ((addr + offset) & 0x000000ff);
}

/**
 * Add a value to an address, BANK WRAPPING
 * @param addr The base address of the operation
 * @param offset The amount to add to the base address
 * @return The addr plus offset, bank wrapped
 */
uint32_t _addr_add_val_bank_wrap(uint32_t addr, uint32_t offset)
{
    return (addr & 0x00ff0000) | ((addr + offset) & 0x0000ffff);
}

/**
 * Get a word from memory, BANK WRAPPING
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to read
 * @return The word in memory at the specified address and address+1, bank wrapped
 */
uint16_t _get_mem_word_bank_wrap(memory_t *mem, uint32_t addr)
{
    uint16_t val = _get_mem_byte(mem, addr);
    val |= _get_mem_byte(mem, _addr_add_val_bank_wrap(addr, 1)) << 8;
    return ;
}

/**
 * Get a byte from memory
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to read
 * @return The byte in memory at the specified address
 */
uint8_t _get_mem_byte(memory_t *mem, uint32_t addr)
{
    return mem[addr]; // Yes, this is simple...
}

/**
 * Get a word from memory
 * @note This WILL NOT perform wrapping under most circumstances.
 *       The only case where wrapping will be performed is when
 *       the low byte is located at address 0x00ffffff. In this case,
 *       the high byte will be read from address 0x00000000.
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to read
 * @return The word in memory at the specified address and address+1
 */
uint16_t _get_mem_word(memory_t *mem, uint32_t addr)
{
    return mem[addr] | (mem[(addr+1) & 0x00ffffff] << 8);
}

/**
 * Set a byte in memory
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to write
 */
void _set_mem_byte(memory_t *mem, uint32_t addr, uint8_t val)
{
    mem[addr] = val; // Yes, this is simple...
}

/**
 * Set a word in memory
 * @note This WILL NOT perform wrapping under most circumstances.
 *       The only case where wrapping will be performed is when
 *       the low byte is located at address 0x00ffffff. In this case,
 *       the high byte will be read from address 0x00000000.
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to write
 */
void _set_mem_word(memory_t *mem, uint32_t addr, uint16_t val)
{
     mem[addr] = val & 0xff;
     mem[(addr + 1) & 0x00ffffff] = val >> 8;
}

/******************************************************
 *                                                    *
 *                 CPU-Addressing Modes               *
 *                                                    *
 ******************************************************/

/**
 * Push a byte (8-bits) onto the CPU's stack
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param byte The byte to be pushed onto the stack
 */
static void _stackCPU_pushByte(CPU_t *cpu, memory_t *mem, uint8_t byte)
{
    _mem_set_byte(mem, cpu->SP, byte);
    _cpu_set_sp(cpu, cpu->SP-1);
}

/**
 * Push a word (16-bits) onto the CPU's stack
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param word The word to be pushed onto the stack
 * @param emulationStack 1 if the stack should be limited to page 1,
 *                       0 for new instructions/native mode
 */
static void _stackCPU_pushWord(CPU_t *cpu, memory_t *mem, uint16_t word, Emul_Stack_Mod_t emulationStack)
{
    if (cpu->P.E && emulationStack)
    {
        // Mem set word? - need to check...
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
static void _stackCPU_push24(CPU_t *cpu, memory_t *mem, int32_t data)
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
static int32_t _stackCPU_popByte(CPU_t *cpu, memory_t *mem, Emul_Stack_Mod_t emulationStack)
{
    int32_t byte = 0;
    if (cpu->P.E && emulationStack)
    {
        cpu->SP = ((cpu->SP + 1) & 0xff) | 0x0100;
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
static int32_t _stackCPU_popWord(CPU_t *cpu, memory_t *mem, Emul_Stack_Mod_t emulationStack)
{
    int32_t word = 0;
    if (cpu->P.E && emulationStack)
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
static int32_t _stackCPU_pop24(CPU_t *cpu, memory_t *mem)
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
static int32_t _addrCPU_getAbsoluteIndexedIndirectX(CPU_t *cpu, memory_t *mem)
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
static int32_t _addrCPU_getAbsoluteIndirect(CPU_t *cpu, memory_t *mem)
{
    // Get the immediate operand word of the current instruction
    int32_t address = ADDR_GET_MEM_IMMD_WORD(cpu, mem);

    // Find and return the resultant indirect address value
    // (from Bank 0)
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
static int32_t _addrCPU_getAbsoluteIndirectLong(CPU_t *cpu, memory_t *mem)
{
    // Get the immediate operand word of the current instruction
    int32_t address = ADDR_GET_MEM_IMMD_WORD(cpu, mem);

    // Find and return the resultant indirect address value
    // (from Bank 0)
    return ADDR_GET_MEM_BYTE(mem, address) | (ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(address, 1)) << 8) |
           (ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(address, 2)) << 16);
}

/**
 * Returns the 24-bit address pointed to the absolute address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @return The 24-bit effective address of the current instruction
 */
static int32_t _addrCPU_getAbsolute(CPU_t *cpu, memory_t *mem)
{
    // Get the immediate operand word of the current instruction
    int32_t address = ADDR_GET_MEM_IMMD_WORD(cpu, mem);

    // Find and return the resultant address value
    return CPU_GET_DBR_SHIFTED(cpu) | address;
}

/**
 * Returns the 24-bit address pointed to the absolute, X-indexed address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @return The 24-bit effective address of the current instruction
 */
static int32_t _addrCPU_getAbsoluteIndexedX(CPU_t *cpu, memory_t *mem)
{
    // Get the immediate operand word of the current instruction and the current data bank
    int32_t address = ADDR_GET_MEM_IMMD_WORD(cpu, mem) | CPU_GET_DBR_SHIFTED(cpu);
    address += cpu->X; // No wraparound

    return address;
}

/**
 * Returns the 24-bit address pointed to the absolute, Y-indexed address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @return The 24-bit effective address of the current instruction
 */
static int32_t _addrCPU_getAbsoluteIndexedY(CPU_t *cpu, memory_t *mem)
{
    // Get the immediate operand word of the current instruction and the current data bank
    int32_t address = ADDR_GET_MEM_IMMD_WORD(cpu, mem) | CPU_GET_DBR_SHIFTED(cpu);
    address += cpu->Y; // No wraparound

    return address;
}

/**
 * Returns the 24-bit address pointed to the long, X-indexed address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @return The 24-bit effective address of the current instruction
 */
static int32_t _addrCPU_getLongIndexedX(CPU_t *cpu, memory_t *mem)
{
    // Get the immediate operand word of the current instruction and the current data bank
    int32_t address = ADDR_GET_MEM_IMMD_LONG(cpu, mem);
    address += cpu->X; // No wraparound

    return address;
}

/**
 * Returns the 24-bit address pointed to the direct page address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @return The 24-bit effective address of the current instruction
 */
static int32_t _addrCPU_getDirectPage(CPU_t *cpu, memory_t *mem)
{
    // Find and return the resultant address value
    return ADDR_ADD_VAL_BANK_WRAP(cpu->D, ADDR_GET_MEM_IMMD_BYTE(cpu, mem));
}

/**
 * Returns the 24-bit address pointed to the (dp) address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @return The 24-bit effective address of the current instruction
 */
static int32_t _addrCPU_getDirectPageIndirect(CPU_t *cpu, memory_t *mem)
{
    // Get the immediate operand word of the current instruction and bank 0
    int32_t address = ADDR_GET_MEM_IMMD_BYTE(cpu, mem);

    if (cpu->P.E && ((cpu->D & 0xff) == 0))
    {
        address = ADDR_ADD_VAL_PAGE_WRAP(cpu->D, address);
        address = ADDR_GET_MEM_BYTE(mem, address) | (ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_PAGE_WRAP(address, 1)) << 8); // 16-bit pointer
    }
    else
    {
        address = ADDR_ADD_VAL_BANK_WRAP(cpu->D, address);
        address = ADDR_GET_MEM_BYTE(mem, address) | (ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(address, 1)) << 8); // 16-bit pointer
    }
    address += CPU_GET_DBR_SHIFTED(cpu);

    return address;
}

/**
 * Returns the 24-bit address pointed to the [dp] address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @return The 24-bit effective address of the current instruction
 */
static int32_t _addrCPU_getDirectPageIndirectLong(CPU_t *cpu, memory_t *mem)
{
    // Get the immediate operand word of the current instruction and bank 0
    int32_t address = ADDR_GET_MEM_IMMD_BYTE(cpu, mem);

    address = ADDR_ADD_VAL_BANK_WRAP(cpu->D, address);
    address = ADDR_GET_MEM_BYTE(mem, address) | (ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(address, 1)) << 8) |
              (ADDR_GET_MEM_BYTE(mem, ADDR_ADD_VAL_BANK_WRAP(address, 2)) << 16); // 24-bit pointer

    return address;
}

/**
 * Returns the 24-bit address pointed to the dp, X-indexed address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @return The 24-bit effective address of the current instruction
 */
static int32_t _addrCPU_getDirectPageIndexedX(CPU_t *cpu, memory_t *mem)
{
    // Get the immediate operand word of the current instruction and bank 0
    int32_t address = ADDR_GET_MEM_IMMD_BYTE(cpu, mem);

    if (cpu->P.E && ((cpu->D & 0xff) == 0))
    {
        address = ADDR_ADD_VAL_PAGE_WRAP(cpu->D, address + cpu->X);
    }
    else
    {
        address += cpu->D;
        address += cpu->X; // No wraparound
    }

    return address & 0xffff; // Inevitable bank wrap
}

/**
 * Returns the 24-bit address pointed to the dp, Y-indexed address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @return The 24-bit effective address of the current instruction
 */
static int32_t _addrCPU_getDirectPageIndexedY(CPU_t *cpu, memory_t *mem)
{
    // Get the immediate operand word of the current instruction and bank 0
    int32_t address = ADDR_GET_MEM_IMMD_BYTE(cpu, mem);

    if (cpu->P.E && ((cpu->D & 0xff) == 0))
    {
        address = ADDR_ADD_VAL_PAGE_WRAP(cpu->D, address + cpu->Y);
    }
    else
    {
        address += cpu->D;
        address += cpu->Y;
    }

    return address & 0xffff; // Inevitable bank wrap
}

/**
 * Returns the 16-bit PC value if a relative-8 branch at the
 * current CPU's PC is taken.
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the relative offset
 * @return The 16-bit PC address as a result of adding the signed
 *         8-bit relative offset
 */
static int32_t _addrCPU_getRelative8(CPU_t *cpu, memory_t *mem)
{
    int16_t offset = ADDR_GET_MEM_IMMD_BYTE(cpu, mem);
    if (offset & 0x80)
    {
        offset |= 0xff00; // Sign extension
    }
    return ADDR_ADD_VAL_BANK_WRAP(ADDR_ADD_VAL_BANK_WRAP(cpu->PC, 2), offset);
}

/**
 * Returns the 16-bit PC value if a relative-16 branch at the
 * current CPU's PC is taken.
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the relative offset
 * @return The 16-bit PC address as a result of adding the signed
 *         16-bit relative offset
 */
static int32_t _addrCPU_getRelative16(CPU_t *cpu, memory_t *mem)
{
    int16_t offset = ADDR_GET_MEM_IMMD_WORD(cpu, mem);
    return ADDR_ADD_VAL_BANK_WRAP(ADDR_ADD_VAL_BANK_WRAP(cpu->PC, 2), offset);
}

/**
 * Returns the 24-bit address pointed to the long address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @return The 24-bit effective address of the current instruction
 */
static int32_t _addrCPU_getLong(CPU_t *cpu, memory_t *mem)
{
    // Get the immediate operand word of the current instruction
    int32_t address = ADDR_GET_MEM_IMMD_LONG(cpu, mem);

    // Find and return the resultant address value
    return address;
}
