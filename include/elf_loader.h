#ifndef RV32I_ELF_LOADER_H
#define RV32I_ELF_LOADER_H

#include <stdint.h>

#include "memory.h"

int rv32i_load_elf(const char *path, rv32i_mem_t *mem, uint32_t *entry_pc);

#endif
