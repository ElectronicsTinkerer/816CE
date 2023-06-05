/**
 * 65(c)816 simulator/emulator (816CE)
 * Copyright (C) 2023 Ray Clemens
 */

#include <stdbool.h>

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
uint32_t _cpu_get_pbr(CPU_t *cpu)
{
    return (uint32_t)cpu->PBR << 16;
}

/**
 * Get the CPU's DATA BANK, shifted to be bits 16..23 of the value
 * @param cpu A pointer to the CPU struct from which the DBR will be retrieved
 * @return The DBR of the given cpu, placed in bits 23..16
 */
uint32_t _cpu_get_dbr(CPU_t *cpu)
{
    return (uint32_t)cpu->DBR << 16;
}

/**
 * Get a CPU's 24 bit PC address
 * @param cpu A pointer to the CPU struct from which to retrieve the 24-bit PC address
 * @return The cpu's PC concatenated with the PBR
 */
uint32_t _cpu_get_effective_pc(CPU_t *cpu)
{
    return _cpu_get_pbr(cpu) | cpu->PC;
}

/**
 * Get the byte in memory at the address CPU PC+1
 * @note This will BANK WRAP
 * @param cpu The CPU from which to retrieve the PC
 * @param mem The memory from which to pull the value
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The value of the byte in memory at the CPU's PC+1
 */
uint8_t _cpu_get_immd_byte(CPU_t *cpu, memory_t *mem, bool setacc)
{
    uint32_t addr = _cpu_get_effective_pc(cpu);
    addr = _addr_add_val_bank_wrap(addr, 1);
    return _get_mem_byte(mem, addr, setacc);
}

/**
 * Get the word in memory at the address CPU PC+1 (high byte @ PC+2)
 * @note This will BANK WRAP
 * @param cpu The CPU from which to retrieve the PC
 * @param mem The memory from which to pull the value
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The value of the word in memory at the CPU's PC+1
 */
uint16_t _cpu_get_immd_word(CPU_t *cpu, memory_t *mem, bool setacc)
{
    uint32_t addr = _cpu_get_effective_pc(cpu);
    addr = _addr_add_val_bank_wrap(addr, 1);
    uint16_t val = _get_mem_byte(mem, addr, setacc);
    addr = _addr_add_val_bank_wrap(addr, 1);
    return val | (_get_mem_byte(mem, addr, setacc) << 8);
}

/**
 * Get the long in memory at the address CPU PC+1 (high byte @ PC+3)
 * @note This will BANK WRAP
 * @param cpu The CPU from which to retrieve the PC
 * @param mem The memory from which to pull the value
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The value of the long in memory at the CPU's PC+1
 */
uint32_t _cpu_get_immd_long(CPU_t *cpu, memory_t *mem, bool setacc)
{
    uint32_t addr = _cpu_get_effective_pc(cpu);
    addr = _addr_add_val_bank_wrap(addr, 1);
    uint32_t val = _get_mem_byte(mem, addr, setacc);
    addr = _addr_add_val_bank_wrap(addr, 1);
    val |= _get_mem_byte(mem, addr, setacc) << 8;
    addr = _addr_add_val_bank_wrap(addr, 1);
    return val | (_get_mem_byte(mem, addr, setacc) << 16);
}

/**
 * Add a value to an address, PAGE WRAPPING
 * @param addr The base address of the operation
 * @param offset The amount to add to the base address
 * @param setacc True to set the "accessed flag" on used memory data
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
 * Set the flag in a specified cpu to indicate that an invalid
 * internal sim error/state was reached
 * @param cpu The CPU which should have its error flag set
 */
void _cpu_crash(CPU_t *cpu)
{
    cpu->P.CRASH = 1;
}

/**
 * Get a byte from memory
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to read
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The byte in memory at the specified address
 */
uint8_t _get_mem_byte(memory_t *mem, uint32_t addr, bool setacc)
{
    if (setacc) {
        mem[addr].acc.R = 1;
    }
    return mem[addr].val; // Yes, this is simple...
}

/**
 * Get a word from memory
 * @note This WILL NOT perform wrapping under most circumstances.
 *       The only case where wrapping will be performed is when
 *       the low byte is located at address 0x00ffffff. In this case,
 *       the high byte will be read from address 0x00000000.
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to read
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The word in memory at the specified address and address+1
 */
uint16_t _get_mem_word(memory_t *mem, uint32_t addr, bool setacc)
{
    if (setacc) {
        mem[addr].acc.R = 1;
        mem[(addr+1) & 0x00ffffff].acc.R = 1;
    }
    return mem[addr].val | (mem[(addr+1) & 0x00ffffff].val << 8);
}

/**
 * Get a word from memory, PAGE WRAPPING
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to read
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The word in memory at the specified address and address+1, page wrapped
 */
uint16_t _get_mem_word_page_wrap(memory_t *mem, uint32_t addr, bool setacc)
{
    uint16_t val = _get_mem_byte(mem, addr, setacc);
    val |= _get_mem_byte(mem, _addr_add_val_page_wrap(addr, 1), setacc) << 8;
    return val;
}

/**
 * Get a word from memory, BANK WRAPPING
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to read
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The word in memory at the specified address and address+1, bank wrapped
 */
uint16_t _get_mem_word_bank_wrap(memory_t *mem, uint32_t addr, bool setacc)
{
    uint16_t val = _get_mem_byte(mem, addr, setacc);
    val |= _get_mem_byte(mem, _addr_add_val_bank_wrap(addr, 1), setacc) << 8;
    return val;
}

/**
 * Get a long from memory, BANK WRAPPING
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to read
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The long in memory at the specified address, address+1, and address+2, bank wrapped
 */
uint32_t _get_mem_long_bank_wrap(memory_t *mem, uint32_t addr, bool setacc)
{
    uint32_t val = _get_mem_byte(mem, addr, setacc);
    val |= _get_mem_byte(mem, _addr_add_val_bank_wrap(addr, 1), setacc) << 8;
    val |= _get_mem_byte(mem, _addr_add_val_bank_wrap(addr, 2), setacc) << 16;
    return val;
}

/**
 * Set a byte in memory
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to write
 * @param setacc True to set the "accessed flag" on used memory data
 * @param val The data value to store
 */
void _set_mem_byte(memory_t *mem, uint32_t addr, uint8_t val, bool setacc)
{
    if (setacc) {
        mem[addr].acc.W = 1;
    }
    mem[addr].val = val; // Yes, this is simple...
}

/**
 * Set a word in memory
 * @note This WILL NOT perform wrapping under most circumstances.
 *       The only case where wrapping will be performed is when
 *       the low byte is located at address 0x00ffffff. In this case,
 *       the high byte will be read from address 0x00000000.
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to write
 * @param setacc True to set the "accessed flag" on used memory data
 * @param val The data value to store
 */
void _set_mem_word(memory_t *mem, uint32_t addr, uint16_t val, bool setacc)
{
    if (setacc) {
        mem[addr].acc.W = 1;
        mem[(addr + 1) & 0x00ffffff].acc.W = 1;
    }
    mem[addr].val = val & 0xff;
    mem[(addr + 1) & 0x00ffffff].val = val >> 8;
}

/**
 * Set a word from memory, BANK WRAPPING
 * @param mem The memory array to use as system memory
 * @param addr The address in memory to read
 * @param setacc True to set the "accessed flag" on used memory data
 * @param val The value to store in memory
 */
void _set_mem_word_bank_wrap(memory_t *mem, uint32_t addr, uint16_t val, bool setacc)
{
    _set_mem_byte(mem, addr, val, setacc);
    _set_mem_byte(mem, _addr_add_val_bank_wrap(addr, 1), val >> 8, setacc);
}

/**
 * Initialize the memory array with a source array
 * 
 * @note This does not modify flag data
 * @param *mem The memory array to save the source data in
 * @param *src The source data to copy into system memory
 * @param base_addr The starting address to copy (for system memory)
 * @param count The number of addresses to copy
 */
void _init_mem_arr(memory_t *mem, uint8_t *src, uint32_t base_addr, uint32_t count)
{
    for (uint32_t i = base_addr, j = 0; j < count; ++i, ++j) {
        mem[i].val = src[j];
    }
}

/**
 * Copy data from memory into a destination buffer
 * 
 * @note this does not copy flag data
 * @param *mem The memory to copy from
 * @param *dst The destination buffer
 * @param base_addr The starting address to copy
 * @param count The number of addresses to copy
 * @return 
 */
void _save_mem_arr(memory_t *mem, uint8_t *dst, uint32_t base_addr, uint32_t count)
{
    for (uint32_t i = base_addr, j = 0; j < count; ++i, ++j) {
        dst[i] = mem[i].val;
    }
}

/**
 * Get the values of the flags on an address without modifying them
 * 
 * @param *mem The memory to read
 * @param addr The address in memory to access
 * @return The flag data present at address
 */
mem_flag_t _test_mem_flags(memory_t *mem, uint32_t addr)
{
    return mem[addr].acc;
}

/**
 * Get the values of the flags on an address then reset them
 * 
 * @param *mem The memory to read
 * @param addr The address in memory to access
 * @param mask A mask to determine which flags are cleared (set to clear)
 *             bit 0: Read flag
 *             bit 1: Write flag
 *             bit 2: Break flag
 *             bit 3..7: Unused
 */
mem_flag_t _test_and_reset_mem_flags(memory_t *mem, uint32_t addr, uint8_t mask)
{
    mem_flag_t t = mem[addr].acc;

    // Flag resetting
    _reset_mem_flags(mem, addr, mask);
    
    return t;
}

/**
 * Reset the values of the flags on an address
 * 
 * @param *mem The memory to modify
 * @param addr The address in memory to access
 * @param mask A mask to determine which flags are reset (set a bit to reset that flag)
 *             bit 0: Read flag
 *             bit 1: Write flag
 *             bit 2: Break flag
 *             bit 3..7: Unused
 */
void _reset_mem_flags(memory_t *mem, uint32_t addr, uint8_t mask)
{
    // Flag resetting
    *(uint8_t *)&(mem[addr].acc) = (~mask) & *(uint8_t *) &(mem[addr].acc);
}

/**
 * Set the values of the flags on an address
 * 
 * @param *mem The memory to modify
 * @param addr The address in memory to access
 * @param mask A mask to determine which flags are set
 *             bit 0: Read flag
 *             bit 1: Write flag
 *             bit 2: Break flag
 *             bit 3..7: Unused
 * @return The flag data present at address prior to calling this function
 */
void _set_mem_flags(memory_t *mem, uint32_t addr, uint8_t mask)
{
    // Flag resetting
    *(uint8_t *)&(mem[addr].acc) = (mask) | *(uint8_t *) &(mem[addr].acc);
}


/******************************************************
 *                                                    *
 *                 CPU-Addressing Modes               *
 *                                                    *
 ******************************************************
 * See: http://6502.org/tutorials/65c816opcodes.html#5
 */

/**
 * Push a byte (8-bits) onto the CPU's stack
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param byte The byte to be pushed onto the stack
 * @param setacc True to set the "accessed flag" on used memory data
 */
void _stackCPU_pushByte(CPU_t *cpu, memory_t *mem, uint8_t byte, bool setacc)
{
    _set_mem_byte(mem, cpu->SP, byte, setacc);
    _cpu_set_sp(cpu, cpu->SP-1);
}

/**
 * Push a word (16-bits) onto the CPU's stack
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param word The word to be pushed onto the stack
 * @param emulationStack 1 if the stack should be limited to page 1 (old instructions),
 *                       0 for new instructions/native mode
 * @param setacc True to set the "accessed flag" on used memory data
 */
void _stackCPU_pushWord(CPU_t *cpu, memory_t *mem, uint16_t word, Emul_Stack_Mod_t emulationStack, bool setacc)
{
    if (cpu->P.E && emulationStack) // Only obey emulationStack when in emulation mode
    {
        _set_mem_byte(mem, cpu->SP, word >> 8, setacc);
        _cpu_set_sp(cpu, cpu->SP - 1);
        _set_mem_byte(mem, cpu->SP, word & 0xff, setacc);
        _cpu_set_sp(cpu, cpu->SP - 1);
    }
    else
    {
        _set_mem_word(mem, _addr_add_val_bank_wrap(cpu->SP, -1), word, setacc);
        _cpu_set_sp(cpu, cpu->SP - 2);
    }
}

/**
 * Push a three bytes (24-bits) onto the CPU's stack
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param data The word to be pushed onto the stack
 * @param setacc True to set the "accessed flag" on used memory data
 */
void _stackCPU_push24(CPU_t *cpu, memory_t *mem, uint32_t data, bool setacc)
{
    _set_mem_byte(mem, cpu->SP, (data >> 16) & 0xff, setacc);
    // Commented code: does not account for word breaks
    // _set_mem_byte(mem, _addr_add_val_bank_wrap(cpu->SP, -1), (data >> 8) & 0xff);
    // _set_mem_byte(mem, _addr_add_val_bank_wrap(cpu->SP, -2), data & 0xff);
    _set_mem_word(mem, _addr_add_val_bank_wrap(cpu->SP, -2), data & 0xffff, setacc);
    _cpu_set_sp(cpu, cpu->SP - 3);
}

/**
 * Pop a byte (8-bits) off the CPU's stack and return it
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param emulationStack 1 if the stack should be limited to page 1 (old instructions),
 *                       0 for new instructions/native mode
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The value popped off the stack
 */
uint8_t _stackCPU_popByte(CPU_t *cpu, memory_t *mem, Emul_Stack_Mod_t emulationStack, bool setacc)
{
    _cpu_set_sp(cpu, cpu->SP + 1);
    return _get_mem_byte(mem, cpu->SP, setacc);
}

/**
 * Pop a word (16-bits) off the CPU's stack and return it
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param emulationStack 1 if the stack should be limited to page 1,
 *                       0 for new instructions/native mode
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The value popped off the stack
 */
uint16_t _stackCPU_popWord(CPU_t *cpu, memory_t *mem, Emul_Stack_Mod_t emulationStack, bool setacc)
{
    uint16_t word = 0;
    if (cpu->P.E && emulationStack) // Only obey emulationStack when in emulation mode
    {
        _cpu_set_sp(cpu, cpu->SP + 1);
        word = _get_mem_byte(mem, cpu->SP, setacc);
        _cpu_set_sp(cpu, cpu->SP + 1);
        word |= _get_mem_byte(mem, cpu->SP, setacc) << 8;
    }
    else
    {
        word = _get_mem_word(mem, _addr_add_val_bank_wrap(cpu->SP, 1), setacc);
        _cpu_set_sp(cpu, cpu->SP + 2);
    }

    return word;
}

/**
 * Pop a triple byte (24-bits) off the CPU's stack and return it
 * @param cpu The cpu to use for the operation
 * @param mem The memory array which is to be connected to the CPU
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The value popped off the stack
 */
uint32_t _stackCPU_pop24(CPU_t *cpu, memory_t *mem, bool setacc)
{
    uint32_t data = 0;

    // Commented code: does not account for word breaks
    // data = _get_mem_byte(mem, _addr_add_val_bank_wrap(cpu->SP, 1));
    // data |= _get_mem_byte(mem, _addr_add_val_bank_wrap(cpu->SP, 2)) << 8;
    data = _get_mem_word(mem, _addr_add_val_bank_wrap(cpu->SP, 1), setacc);
    data |= _get_mem_byte(mem, _addr_add_val_bank_wrap(cpu->SP, 3), setacc) << 16;
    _cpu_set_sp(cpu, cpu->SP + 3);

    return data;
}

/**
 * Returns the 16-bit word in memory stored at the addr,X
 * from the current instruction.(i.e. the PC part of the resultant
 * indirect address)
 * This will BANK WRAP when reading the word from memory
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the indirect address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The word in memory at the indirect address (in the current PRB bank)
 */
uint16_t _addrCPU_getAbsoluteIndexedIndirectX(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction
    uint32_t address = _cpu_get_immd_word(cpu, mem, setacc);
    address += cpu->X;
    address &= 0xffff; // Wraparound
    address |= _cpu_get_pbr(cpu);

    // Find and return the resultant indirect address value
    uint16_t data = _get_mem_word_bank_wrap(mem, address, setacc);
    return data;
}

/**
 * Returns the 16-bit word in memory stored at the addr
 * from the current instruction. (i.e. the PC part of the resultant
 * indirect address)
 * This will BANK WRAP when reading the word from memory
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the indirect address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The word in memory at the indirect address (in bank 0)
 */
uint16_t _addrCPU_getAbsoluteIndirect(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction
    uint32_t address = _cpu_get_immd_word(cpu, mem, setacc);

    // Find and return the resultant indirect address value
    // (from Bank 0)
    uint16_t data = _get_mem_word_bank_wrap(mem, address, setacc);
    return data;
}

/**
 * Returns the 24-bit word in memory stored at the addr
 * from the current instruction. (i.e. the PC part of the resultant
 * indirect address)
 * This will BANK WRAP when reading the word from memory
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the indirect address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The word and byte in memory at the indirect address (in bank 0)
 */
uint32_t _addrCPU_getAbsoluteIndirectLong(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction
    uint32_t address = _cpu_get_immd_word(cpu, mem, setacc);

    // Find and return the resultant indirect address value
    // (from Bank 0)
    uint32_t data = _get_mem_long_bank_wrap(mem, address, setacc);
    return data;
}

/**
 * Returns the 24-bit address pointed to the absolute address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction
 */
uint32_t _addrCPU_getAbsolute(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction
    uint32_t address = _cpu_get_immd_word(cpu, mem, setacc);

    // Find and return the resultant address value
    return _cpu_get_dbr(cpu) | address;
}

/**
 * Returns the 24-bit address pointed to the absolute, X-indexed address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction
 */
uint32_t _addrCPU_getAbsoluteIndexedX(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction and the current data bank
    uint32_t address = _cpu_get_immd_word(cpu, mem, setacc) | _cpu_get_dbr(cpu);
    address += cpu->X; // No wraparound
    return address & 0xffffff;
}

/**
 * Returns the 24-bit address pointed to the absolute, Y-indexed address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction
 */
uint32_t _addrCPU_getAbsoluteIndexedY(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction and the current data bank
    uint32_t address = _cpu_get_immd_word(cpu, mem, setacc) | _cpu_get_dbr(cpu);
    address += cpu->Y; // No wraparound
    return address & 0xffffff;
}

/**
 * Returns the 24-bit address pointed to the long, X-indexed address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction's operand
 */
uint32_t _addrCPU_getLongIndexedX(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction and the current data bank
    uint32_t address = _cpu_get_immd_long(cpu, mem, setacc);
    address += cpu->X; // No wraparound
    return address & 0xffffff;
}

/**
 * Returns the 24-bit address pointed to by the direct page address of
 * the current instruction's operand (always bank 0)
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction
 */
uint32_t _addrCPU_getDirectPage(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Find and return the resultant address value
    return _addr_add_val_bank_wrap(cpu->D, _cpu_get_immd_byte(cpu, mem, setacc));
}

/**
 * Returns the 24-bit address pointed to by the dp address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction
 */
uint32_t _addrCPU_getDirectPageIndirect(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand byte of the current instruction and bank 0
    uint32_t address = _cpu_get_immd_byte(cpu, mem, setacc);

    if (cpu->P.E && ((cpu->D & 0xff) == 0))
    {
        address = _addr_add_val_page_wrap(cpu->D, address);
        address = _get_mem_word_bank_wrap(mem, address, setacc); // 16-bit pointer
    }
    else
    {
        address = _addr_add_val_page_wrap(cpu->D, address);
        address = _get_mem_word_bank_wrap(mem, address, setacc); // 16-bit pointer
    }
    address |= _cpu_get_dbr(cpu);

    return address;
}

/**
 * Returns the 24-bit address pointed to by the [dp] address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction
 */
uint32_t _addrCPU_getDirectPageIndirectLong(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction and bank 0
    uint32_t address = _cpu_get_immd_byte(cpu, mem, setacc);

    address = _addr_add_val_bank_wrap(cpu->D, address);
    address = _get_mem_long_bank_wrap(mem, address, setacc); // 24-bit pointer

    return address;
}

/**
 * Returns the 24-bit address pointed to by the dp, X-indexed address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction
 */
uint32_t _addrCPU_getDirectPageIndexedX(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction and bank 0
    uint32_t address = _cpu_get_immd_byte(cpu, mem, setacc);

    if (cpu->P.E && ((cpu->D & 0xff) == 0))
    {
        address = _addr_add_val_page_wrap(cpu->D, address + cpu->X);
    }
    else
    {
        address = _addr_add_val_bank_wrap(address, cpu->D);
        address = _addr_add_val_bank_wrap(address, cpu->X);
    }

    return address;
}

/**
 * Returns the 24-bit address pointed to by the (dp,X) address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction
 */
uint32_t _addrCPU_getDirectPageIndexedIndirectX(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction and bank 0
    uint32_t address = _cpu_get_immd_byte(cpu, mem, setacc);

    if (cpu->P.E && ((cpu->D & 0xff) == 0))
    {
        address = _addr_add_val_page_wrap(cpu->D, address + cpu->X);
        address = _get_mem_word_bank_wrap(mem, address, setacc);
    }
    else
    {
        address = _addr_add_val_bank_wrap(address, cpu->D + cpu->X);
        address = _get_mem_word_bank_wrap(mem, address, setacc);
    }

    address |= _cpu_get_dbr(cpu);

    return address;
}

/**
 * Returns the 24-bit address pointed to by the dp, Y-indexed address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction
 */
uint32_t _addrCPU_getDirectPageIndexedY(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction and bank 0
    uint32_t address = _cpu_get_immd_byte(cpu, mem, setacc);

    if (cpu->P.E && ((cpu->D & 0xff) == 0))
    {
        address = _addr_add_val_page_wrap(cpu->D, address + cpu->Y);
    }
    else
    {
        _addr_add_val_bank_wrap(address, cpu->D);
        _addr_add_val_bank_wrap(address, cpu->Y);
    }

    return address;
}

/**
 * Returns the 24-bit address pointed to by the (dp),Y address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction
 */
uint32_t _addrCPU_getDirectPageIndirectIndexedY(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction and bank 0
    uint32_t address = _cpu_get_immd_byte(cpu, mem, setacc);

    address = _addr_add_val_bank_wrap(cpu->D, address);

    if (cpu->P.E && ((cpu->D & 0xff) == 0))
    {
        address = _get_mem_word_page_wrap(mem, address, setacc);
    }
    else
    {
        address = _get_mem_word_bank_wrap(mem, address, setacc);
    }

    address |= _cpu_get_dbr(cpu);
    address += cpu->Y;
    address &= 0xffffff;

    return address;
}

/**
 * Returns the 24-bit address pointed to by the [dp],Y address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction
 */
uint32_t _addrCPU_getDirectPageIndirectLongIndexedY(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction and bank 0
    uint32_t address = _cpu_get_immd_byte(cpu, mem, setacc);

    address = _addr_add_val_bank_wrap(cpu->D, address);

    // Get pointer
    address = _get_mem_long_bank_wrap(mem, address, setacc);

    address += cpu->Y;
    address &= 0xffffff;

    return address;
}

/**
 * Returns the 16-bit PC value of a relative-8 branch at the
 * current CPU's PC is taken.
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the relative offset
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 16-bit PC address as a result of adding the signed
 *         8-bit relative offset
 */
uint32_t _addrCPU_getRelative8(CPU_t *cpu, memory_t *mem, bool setacc)
{
    uint32_t offset = _cpu_get_immd_byte(cpu, mem, setacc);
    if (offset & 0x80)
    {
        offset |= 0xffffff00; // Sign extension
    }
    uint32_t address = _cpu_get_effective_pc(cpu);
    address = _addr_add_val_bank_wrap(address, 2);
    address = _addr_add_val_bank_wrap(address, offset);
    return address;
}

/**
 * Returns the 16-bit PC value of a relative-16 branch at the
 * current CPU's PC is taken.
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the relative offset
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 16-bit PC address as a result of adding the signed
 *         16-bit relative offset
 */
uint32_t _addrCPU_getRelative16(CPU_t *cpu, memory_t *mem, bool setacc)
{
    uint32_t offset = _cpu_get_immd_byte(cpu, mem, setacc);
    if (offset & 0x8000)
    {
        offset |= 0xffff0000; // Sign extension
    }
    uint32_t address = _cpu_get_effective_pc(cpu);
    address = _addr_add_val_bank_wrap(address, 3);
    address = _addr_add_val_bank_wrap(address, offset);
    return address;
}

/**
 * Returns the 24-bit address pointed to the long address of
 * the current instruction's operand
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The 24-bit effective address of the current instruction
 */
uint32_t _addrCPU_getLong(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction
    uint32_t address = _cpu_get_immd_long(cpu, mem, setacc);
    return address;
}

/**
 * Returns the address of the operand to the current instruction
 * (PC+1)
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The address of the immediate operand of the current instruction
 */
uint32_t _addrCPU_getImmediate(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction
    uint32_t address = _cpu_get_effective_pc(cpu);
    address = _addr_add_val_bank_wrap(address, 1);
    return address;
}

/**
 * Returns the effective stack relative address of the current instruction
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The SR address of the current instruction
 */
uint32_t _addrCPU_getStackRelative(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction
    uint32_t address = _cpu_get_immd_byte(cpu, mem, setacc);
    return _addr_add_val_bank_wrap(cpu->SP, address);
}

/**
 * Returns the effective stack relative address of the current instruction
 * @param cpu The cpu to use for the operation
 * @param mem The memory which will provide the operand address
 * @param setacc True to set the "accessed flag" on used memory data
 * @return The SR address of the current instruction
 */
uint32_t _addrCPU_getStackRelativeIndirectIndexedY(CPU_t *cpu, memory_t *mem, bool setacc)
{
    // Get the immediate operand word of the current instruction
    uint32_t address = _cpu_get_immd_byte(cpu, mem, setacc);
    address = _addr_add_val_bank_wrap(cpu->SP, address); // Calculate pointer offset
    address = _get_mem_word_bank_wrap(mem, address, setacc); // Get pointer
    address += cpu->Y;
    return  address & 0xffffff;
}


