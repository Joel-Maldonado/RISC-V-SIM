#include "memory.h"

#include <stdlib.h>
#include <string.h>

static int translate(const rv32i_mem_t *mem, uint32_t addr, size_t len, size_t *offset_out) {
    uint64_t start;
    if (mem == NULL || mem->data == NULL) {
        return RV32I_MEM_ERR_BOUNDS;
    }
    if (addr < mem->base) {
        return RV32I_MEM_ERR_BOUNDS;
    }

    start = (uint64_t)addr - (uint64_t)mem->base;
    if (start + len > mem->size) {
        return RV32I_MEM_ERR_BOUNDS;
    }

    if (offset_out != NULL) {
        *offset_out = (size_t)start;
    }
    return RV32I_MEM_OK;
}

int rv32i_mem_init(rv32i_mem_t *mem, uint32_t base, uint32_t size) {
    if (mem == NULL || size == 0U) {
        return RV32I_MEM_ERR_BOUNDS;
    }

    mem->data = (uint8_t *)calloc(size, sizeof(uint8_t));
    if (mem->data == NULL) {
        return RV32I_MEM_ERR_BOUNDS;
    }

    mem->base = base;
    mem->size = size;
    return RV32I_MEM_OK;
}

void rv32i_mem_free(rv32i_mem_t *mem) {
    if (mem == NULL) {
        return;
    }
    free(mem->data);
    mem->data = NULL;
    mem->size = 0U;
    mem->base = 0U;
}

int rv32i_mem_read8(const rv32i_mem_t *mem, uint32_t addr, uint8_t *value) {
    size_t offset;
    int rc = translate(mem, addr, 1U, &offset);
    if (rc != RV32I_MEM_OK) {
        return rc;
    }
    *value = mem->data[offset];
    return RV32I_MEM_OK;
}

int rv32i_mem_read16(const rv32i_mem_t *mem, uint32_t addr, uint16_t *value) {
    size_t offset;
    int rc;

    if ((addr & 0x1U) != 0U) {
        return RV32I_MEM_ERR_MISALIGNED;
    }

    rc = translate(mem, addr, 2U, &offset);
    if (rc != RV32I_MEM_OK) {
        return rc;
    }

    *value = (uint16_t)mem->data[offset] | ((uint16_t)mem->data[offset + 1U] << 8);
    return RV32I_MEM_OK;
}

int rv32i_mem_read32(const rv32i_mem_t *mem, uint32_t addr, uint32_t *value) {
    size_t offset;
    int rc;

    if ((addr & 0x3U) != 0U) {
        return RV32I_MEM_ERR_MISALIGNED;
    }

    rc = translate(mem, addr, 4U, &offset);
    if (rc != RV32I_MEM_OK) {
        return rc;
    }

    *value = (uint32_t)mem->data[offset] |
             ((uint32_t)mem->data[offset + 1U] << 8) |
             ((uint32_t)mem->data[offset + 2U] << 16) |
             ((uint32_t)mem->data[offset + 3U] << 24);
    return RV32I_MEM_OK;
}

int rv32i_mem_write8(rv32i_mem_t *mem, uint32_t addr, uint8_t value) {
    size_t offset;
    int rc = translate(mem, addr, 1U, &offset);
    if (rc != RV32I_MEM_OK) {
        return rc;
    }
    mem->data[offset] = value;
    return RV32I_MEM_OK;
}

int rv32i_mem_write16(rv32i_mem_t *mem, uint32_t addr, uint16_t value) {
    size_t offset;
    int rc;

    if ((addr & 0x1U) != 0U) {
        return RV32I_MEM_ERR_MISALIGNED;
    }

    rc = translate(mem, addr, 2U, &offset);
    if (rc != RV32I_MEM_OK) {
        return rc;
    }

    mem->data[offset] = (uint8_t)(value & 0xffU);
    mem->data[offset + 1U] = (uint8_t)((value >> 8) & 0xffU);
    return RV32I_MEM_OK;
}

int rv32i_mem_write32(rv32i_mem_t *mem, uint32_t addr, uint32_t value) {
    size_t offset;
    int rc;

    if ((addr & 0x3U) != 0U) {
        return RV32I_MEM_ERR_MISALIGNED;
    }

    rc = translate(mem, addr, 4U, &offset);
    if (rc != RV32I_MEM_OK) {
        return rc;
    }

    mem->data[offset] = (uint8_t)(value & 0xffU);
    mem->data[offset + 1U] = (uint8_t)((value >> 8) & 0xffU);
    mem->data[offset + 2U] = (uint8_t)((value >> 16) & 0xffU);
    mem->data[offset + 3U] = (uint8_t)((value >> 24) & 0xffU);
    return RV32I_MEM_OK;
}

int rv32i_mem_load_bytes(rv32i_mem_t *mem, uint32_t addr, const uint8_t *src, size_t len) {
    size_t offset;
    int rc;

    if (len == 0U) {
        return RV32I_MEM_OK;
    }
    if (src == NULL) {
        return RV32I_MEM_ERR_BOUNDS;
    }

    rc = translate(mem, addr, len, &offset);
    if (rc != RV32I_MEM_OK) {
        return rc;
    }

    memcpy(mem->data + offset, src, len);
    return RV32I_MEM_OK;
}

int rv32i_mem_zero_bytes(rv32i_mem_t *mem, uint32_t addr, size_t len) {
    size_t offset;
    int rc;

    if (len == 0U) {
        return RV32I_MEM_OK;
    }

    rc = translate(mem, addr, len, &offset);
    if (rc != RV32I_MEM_OK) {
        return rc;
    }

    memset(mem->data + offset, 0, len);
    return RV32I_MEM_OK;
}
