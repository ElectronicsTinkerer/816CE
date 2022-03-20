
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
