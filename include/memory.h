#ifndef RV32I_MEMORY_H
#define RV32I_MEMORY_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *data;
    uint32_t size;
    uint32_t base;
} rv32i_mem_t;

enum {
    RV32I_MEM_OK = 0,
    RV32I_MEM_ERR_BOUNDS = -1,
    RV32I_MEM_ERR_MISALIGNED = -2
};

int rv32i_mem_init(rv32i_mem_t *mem, uint32_t base, uint32_t size);
void rv32i_mem_free(rv32i_mem_t *mem);

int rv32i_mem_read8(const rv32i_mem_t *mem, uint32_t addr, uint8_t *value);
int rv32i_mem_read16(const rv32i_mem_t *mem, uint32_t addr, uint16_t *value);
int rv32i_mem_read32(const rv32i_mem_t *mem, uint32_t addr, uint32_t *value);

int rv32i_mem_write8(rv32i_mem_t *mem, uint32_t addr, uint8_t value);
int rv32i_mem_write16(rv32i_mem_t *mem, uint32_t addr, uint16_t value);
int rv32i_mem_write32(rv32i_mem_t *mem, uint32_t addr, uint32_t value);

int rv32i_mem_load_bytes(rv32i_mem_t *mem, uint32_t addr, const uint8_t *src, size_t len);
int rv32i_mem_zero_bytes(rv32i_mem_t *mem, uint32_t addr, size_t len);

#endif
