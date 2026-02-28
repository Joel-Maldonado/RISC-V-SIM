#include "elf_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elf32.h"
static int validate_header(const Elf32_Ehdr *hdr) {
    if (memcmp(hdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "ELF loader error: invalid ELF magic\n");
        return -1;
    }
    if (hdr->e_ident[EI_CLASS] != ELFCLASS32) {
        fprintf(stderr, "ELF loader error: expected ELF32 input\n");
        return -1;
    }
    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        fprintf(stderr, "ELF loader error: expected little-endian ELF\n");
        return -1;
    }
    if (hdr->e_machine != EM_RISCV) {
        fprintf(stderr, "ELF loader error: expected RISC-V machine type\n");
        return -1;
    }
    if (hdr->e_type != ET_EXEC) {
        fprintf(stderr, "ELF loader error: expected executable ELF (ET_EXEC)\n");
        return -1;
    }
    if (hdr->e_phnum == 0U) {
        fprintf(stderr, "ELF loader error: no program headers found\n");
        return -1;
    }
    if (hdr->e_phentsize != sizeof(Elf32_Phdr)) {
        fprintf(stderr, "ELF loader error: unexpected program header entry size\n");
        return -1;
    }
    return 0;
}

static int segment_fits_memory(const rv32i_mem_t *mem, uint32_t vaddr, uint32_t memsz) {
    uint64_t mem_base = mem->base;
    uint64_t mem_end = mem_base + mem->size;
    uint64_t seg_start = vaddr;
    uint64_t seg_end = seg_start + memsz;
    return seg_start >= mem_base && seg_end <= mem_end;
}

int rv32i_load_elf(const char *path, rv32i_mem_t *mem, uint32_t *entry_pc) {
    FILE *fp = NULL;
    Elf32_Ehdr ehdr;
    uint16_t idx;

    if (path == NULL || mem == NULL || mem->data == NULL || entry_pc == NULL) {
        fprintf(stderr, "ELF loader error: invalid input parameters\n");
        return -1;
    }

    fp = fopen(path, "rb");
    if (fp == NULL) {
        perror("ELF loader error: failed to open file");
        return -1;
    }

    if (fread(&ehdr, sizeof(ehdr), 1U, fp) != 1U) {
        fprintf(stderr, "ELF loader error: failed to read ELF header\n");
        fclose(fp);
        return -1;
    }

    if (validate_header(&ehdr) != 0) {
        fclose(fp);
        return -1;
    }

    memset(mem->data, 0, mem->size);

    for (idx = 0; idx < ehdr.e_phnum; ++idx) {
        Elf32_Phdr phdr;
        long phoff = (long)ehdr.e_phoff + (long)idx * (long)ehdr.e_phentsize;

        if (fseek(fp, phoff, SEEK_SET) != 0) {
            fprintf(stderr, "ELF loader error: failed to seek to program header %u\n", idx);
            fclose(fp);
            return -1;
        }

        if (fread(&phdr, sizeof(phdr), 1U, fp) != 1U) {
            fprintf(stderr, "ELF loader error: failed to read program header %u\n", idx);
            fclose(fp);
            return -1;
        }

        if (phdr.p_type != PT_LOAD || phdr.p_memsz == 0U) {
            continue;
        }

        if (phdr.p_memsz < phdr.p_filesz) {
            fprintf(stderr, "ELF loader error: segment %u has memsz < filesz\n", idx);
            fclose(fp);
            return -1;
        }

        if (!segment_fits_memory(mem, phdr.p_vaddr, phdr.p_memsz)) {
            fprintf(stderr,
                    "ELF loader error: segment %u out of memory bounds (vaddr=0x%08x, memsz=%u)\n",
                    idx,
                    phdr.p_vaddr,
                    phdr.p_memsz);
            fclose(fp);
            return -1;
        }

        if (phdr.p_filesz > 0U) {
            uint8_t *buffer = (uint8_t *)malloc(phdr.p_filesz);
            if (buffer == NULL) {
                fprintf(stderr, "ELF loader error: allocation failure for segment %u\n", idx);
                fclose(fp);
                return -1;
            }

            if (fseek(fp, (long)phdr.p_offset, SEEK_SET) != 0) {
                fprintf(stderr, "ELF loader error: failed to seek segment %u data\n", idx);
                free(buffer);
                fclose(fp);
                return -1;
            }

            if (fread(buffer, 1U, phdr.p_filesz, fp) != phdr.p_filesz) {
                fprintf(stderr, "ELF loader error: failed to read segment %u bytes\n", idx);
                free(buffer);
                fclose(fp);
                return -1;
            }

            if (rv32i_mem_load_bytes(mem, phdr.p_vaddr, buffer, phdr.p_filesz) != RV32I_MEM_OK) {
                fprintf(stderr, "ELF loader error: failed to load segment %u into memory\n", idx);
                free(buffer);
                fclose(fp);
                return -1;
            }

            free(buffer);
        }

        if (phdr.p_memsz > phdr.p_filesz) {
            uint32_t zero_start = phdr.p_vaddr + phdr.p_filesz;
            size_t zero_size = (size_t)(phdr.p_memsz - phdr.p_filesz);
            if (rv32i_mem_zero_bytes(mem, zero_start, zero_size) != RV32I_MEM_OK) {
                fprintf(stderr, "ELF loader error: failed to zero BSS for segment %u\n", idx);
                fclose(fp);
                return -1;
            }
        }
    }

    *entry_pc = ehdr.e_entry;
    fclose(fp);
    return 0;
}
