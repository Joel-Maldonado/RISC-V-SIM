#ifndef RV32I_CPU_H
#define RV32I_CPU_H

#include <stdint.h>

#include "memory.h"

enum {
    RV32I_TRAP_NONE = 0,
    RV32I_TRAP_FETCH_BOUNDS = 1,
    RV32I_TRAP_FETCH_MISALIGNED = 2,
    RV32I_TRAP_LOAD_BOUNDS = 3,
    RV32I_TRAP_STORE_BOUNDS = 4,
    RV32I_TRAP_MISALIGNED_ACCESS = 5,
    RV32I_TRAP_ILLEGAL_INSTRUCTION = 6,
    RV32I_TRAP_UNSUPPORTED_ECALL = 7,
    RV32I_TRAP_BREAKPOINT = 8,
    RV32I_TRAP_STEP_LIMIT = 9
};

typedef struct {
    uint32_t regs[32];
    uint32_t pc;
    uint64_t steps;
    int halted;
    int exit_code;
    int trap_code;
} rv32i_cpu_t;

typedef struct {
    int halted;
    int exit_code;
    int trap_code;
    uint64_t steps;
} rv32i_run_result_t;

void rv32i_cpu_init(rv32i_cpu_t *cpu, uint32_t initial_pc, uint32_t initial_sp);
rv32i_run_result_t rv32i_run(rv32i_cpu_t *cpu, rv32i_mem_t *mem, uint64_t max_steps, int trace);
void rv32i_dump_registers(const rv32i_cpu_t *cpu);
const char *rv32i_trap_name(int trap_code);

#endif
