
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
 * Get the CPU's DATA BANK, shifted to be bits 16..23 of the value
 * @param cpu A pointer to the CPU struct from which the DBR will be retrieved
 * @return The CBR of the given cpu, placed in bits 23..16
 */
uint32_t _cpu_get_dbr(CPU_t * cpu, uint8_t sr)
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


