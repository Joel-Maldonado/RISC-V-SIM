#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "elf_loader.h"
#include "memory.h"

#define RV32I_RAM_BASE 0x00000000U
#define RV32I_RAM_SIZE (128U * 1024U)

static void print_usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s <elf-path> [--max-steps N] [--dump-regs] [--trace]\n",
            prog);
}

static int parse_u64(const char *text, uint64_t *out) {
    char *end = NULL;
    unsigned long long value;
    const unsigned char *p = (const unsigned char *)text;

    while (*p != '\0' && isspace(*p)) {
        ++p;
    }
    if (*p == '-') {
        return -1;
    }

    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return -1;
    }
    *out = (uint64_t)value;
    return 0;
}

int main(int argc, char **argv) {
    const char *elf_path;
    uint64_t max_steps = 1000000U;
    int dump_regs = 0;
    int trace = 0;
    int idx;
    rv32i_mem_t mem;
    rv32i_cpu_t cpu;
    rv32i_run_result_t result;
    uint32_t entry_pc = 0U;
    uint32_t initial_sp;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    elf_path = argv[1];
    for (idx = 2; idx < argc; ++idx) {
        if (strcmp(argv[idx], "--dump-regs") == 0) {
            dump_regs = 1;
            continue;
        }
        if (strcmp(argv[idx], "--trace") == 0) {
            trace = 1;
            continue;
        }
        if (strcmp(argv[idx], "--max-steps") == 0) {
            if (idx + 1 >= argc) {
                fprintf(stderr, "Missing value for --max-steps\n");
                return 1;
            }
            if (parse_u64(argv[idx + 1], &max_steps) != 0) {
                fprintf(stderr, "Invalid --max-steps value: %s\n", argv[idx + 1]);
                return 1;
            }
            idx += 1;
            continue;
        }

        fprintf(stderr, "Unknown argument: %s\n", argv[idx]);
        print_usage(argv[0]);
        return 1;
    }

    if (rv32i_mem_init(&mem, RV32I_RAM_BASE, RV32I_RAM_SIZE) != RV32I_MEM_OK) {
        fprintf(stderr, "Failed to initialize simulator memory\n");
        return 1;
    }

    if (rv32i_load_elf(elf_path, &mem, &entry_pc) != 0) {
        rv32i_mem_free(&mem);
        return 1;
    }

    initial_sp = (mem.base + mem.size) & ~0xFU;
    rv32i_cpu_init(&cpu, entry_pc, initial_sp);
    result = rv32i_run(&cpu, &mem, max_steps, trace);

    printf("HALTED: %s\n", result.halted ? "yes" : "no");
    printf("HALT_REASON: %s\n", rv32i_trap_name(result.trap_code));
    printf("EXIT_CODE: %d\n", result.exit_code);
    printf("TRAP_CODE: %d\n", result.trap_code);
    printf("STEPS: %" PRIu64 "\n", result.steps);
    printf("FINAL_PC: 0x%08" PRIx32 "\n", cpu.pc);

    if (dump_regs) {
        rv32i_dump_registers(&cpu);
    }

    rv32i_mem_free(&mem);

    if (result.trap_code == RV32I_TRAP_NONE) {
        return 0;
    }
    return 2;
}
