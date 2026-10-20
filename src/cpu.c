#include "cpu.h"

#include <inttypes.h>
#include <stdio.h>

#include "decode.h"

static const char *k_abi_names[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

static void raise_trap(rv32i_cpu_t *cpu, int trap_code) {
    cpu->halted = 1;
    cpu->trap_code = trap_code;
}

static int map_fetch_error(int mem_rc) {
    if (mem_rc == RV32I_MEM_ERR_MISALIGNED) {
        return RV32I_TRAP_FETCH_MISALIGNED;
    }
    return RV32I_TRAP_FETCH_BOUNDS;
}

static int map_load_error(int mem_rc) {
    if (mem_rc == RV32I_MEM_ERR_MISALIGNED) {
        return RV32I_TRAP_MISALIGNED_ACCESS;
    }
    return RV32I_TRAP_LOAD_BOUNDS;
}

static int map_store_error(int mem_rc) {
    if (mem_rc == RV32I_MEM_ERR_MISALIGNED) {
        return RV32I_TRAP_MISALIGNED_ACCESS;
    }
    return RV32I_TRAP_STORE_BOUNDS;
}

void rv32i_cpu_init(rv32i_cpu_t *cpu, uint32_t initial_pc, uint32_t initial_sp) {
    unsigned i;
    if (cpu == NULL) {
        return;
    }

    for (i = 0; i < 32U; ++i) {
        cpu->regs[i] = 0U;
    }
    cpu->pc = initial_pc;
    cpu->steps = 0U;
    cpu->halted = 0;
    cpu->exit_code = 0;
    cpu->trap_code = RV32I_TRAP_NONE;
    cpu->regs[2] = initial_sp;
}

const char *rv32i_trap_name(int trap_code) {
    switch (trap_code) {
        case RV32I_TRAP_NONE:
            return "ECALL_EXIT";
        case RV32I_TRAP_FETCH_BOUNDS:
            return "FETCH_BOUNDS";
        case RV32I_TRAP_FETCH_MISALIGNED:
            return "FETCH_MISALIGNED";
        case RV32I_TRAP_LOAD_BOUNDS:
            return "LOAD_BOUNDS";
        case RV32I_TRAP_STORE_BOUNDS:
            return "STORE_BOUNDS";
        case RV32I_TRAP_MISALIGNED_ACCESS:
            return "MISALIGNED_ACCESS";
        case RV32I_TRAP_ILLEGAL_INSTRUCTION:
            return "ILLEGAL_INSTRUCTION";
        case RV32I_TRAP_UNSUPPORTED_ECALL:
            return "UNSUPPORTED_ECALL";
        case RV32I_TRAP_BREAKPOINT:
            return "BREAKPOINT";
        case RV32I_TRAP_STEP_LIMIT:
            return "STEP_LIMIT";
        default:
            return "UNKNOWN_TRAP";
    }
}

void rv32i_dump_registers(const rv32i_cpu_t *cpu) {
    unsigned i;
    for (i = 0; i < 32U; ++i) {
        printf("x%02u (%3s): 0x%08" PRIx32 "\n", i, k_abi_names[i], cpu->regs[i]);
    }
}

rv32i_run_result_t rv32i_run(rv32i_cpu_t *cpu, rv32i_mem_t *mem, uint64_t max_steps, int trace) {
    rv32i_run_result_t result = {0, 0, RV32I_TRAP_NONE, 0};

    while (!cpu->halted) {
        uint32_t instruction;
        uint32_t opcode;
        uint32_t rd;
        uint32_t funct3;
        uint32_t rs1;
        uint32_t rs2;
        uint32_t funct7;
        uint32_t next_pc;
        int advance_pc = 1;
        int rc;

        if (max_steps > 0U && cpu->steps >= max_steps) {
            raise_trap(cpu, RV32I_TRAP_STEP_LIMIT);
            break;
        }

        if ((cpu->pc & 0x3U) != 0U) {
            raise_trap(cpu, RV32I_TRAP_FETCH_MISALIGNED);
            break;
        }

        rc = rv32i_mem_read32(mem, cpu->pc, &instruction);
        if (rc != RV32I_MEM_OK) {
            raise_trap(cpu, map_fetch_error(rc));
            break;
        }

        opcode = rv32i_get_opcode(instruction);
        rd = rv32i_get_rd(instruction);
        funct3 = rv32i_get_funct3(instruction);
        rs1 = rv32i_get_rs1(instruction);
        rs2 = rv32i_get_rs2(instruction);
        funct7 = rv32i_get_funct7(instruction);
        next_pc = cpu->pc + 4U;

        if (trace) {
            printf("TRACE pc=0x%08" PRIx32 " inst=0x%08" PRIx32 " %-8s\n",
                   cpu->pc,
                   instruction,
                   rv32i_mnemonic(instruction));
        }

        switch (opcode) {
            case 0x33:
                switch (funct3) {
                    case 0x0:
                        if (funct7 == 0x00U) {
                            cpu->regs[rd] = cpu->regs[rs1] + cpu->regs[rs2];
                        } else if (funct7 == 0x20U) {
                            cpu->regs[rd] = cpu->regs[rs1] - cpu->regs[rs2];
                        } else {
                            raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                            advance_pc = 0;
                        }
                        break;
                    case 0x1:
                        if (funct7 == 0x00U) {
                            cpu->regs[rd] = cpu->regs[rs1] << (cpu->regs[rs2] & 0x1fU);
                        } else {
                            raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                            advance_pc = 0;
                        }
                        break;
                    case 0x2:
                        if (funct7 == 0x00U) {
                            cpu->regs[rd] = ((int32_t)cpu->regs[rs1] < (int32_t)cpu->regs[rs2]) ? 1U : 0U;
                        } else {
                            raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                            advance_pc = 0;
                        }
                        break;
                    case 0x3:
                        if (funct7 == 0x00U) {
                            cpu->regs[rd] = (cpu->regs[rs1] < cpu->regs[rs2]) ? 1U : 0U;
                        } else {
                            raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                            advance_pc = 0;
                        }
                        break;
                    case 0x4:
                        if (funct7 == 0x00U) {
                            cpu->regs[rd] = cpu->regs[rs1] ^ cpu->regs[rs2];
                        } else {
                            raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                            advance_pc = 0;
                        }
                        break;
                    case 0x5:
                        if (funct7 == 0x00U) {
                            cpu->regs[rd] = cpu->regs[rs1] >> (cpu->regs[rs2] & 0x1fU);
                        } else if (funct7 == 0x20U) {
                            cpu->regs[rd] = (uint32_t)((int32_t)cpu->regs[rs1] >> (cpu->regs[rs2] & 0x1fU));
                        } else {
                            raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                            advance_pc = 0;
                        }
                        break;
                    case 0x6:
                        if (funct7 == 0x00U) {
                            cpu->regs[rd] = cpu->regs[rs1] | cpu->regs[rs2];
                        } else {
                            raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                            advance_pc = 0;
                        }
                        break;
                    case 0x7:
                        if (funct7 == 0x00U) {
                            cpu->regs[rd] = cpu->regs[rs1] & cpu->regs[rs2];
                        } else {
                            raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                            advance_pc = 0;
                        }
                        break;
                    default:
                        raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                        advance_pc = 0;
                        break;
                }
                break;

            case 0x13: {
                int32_t imm_i = rv32i_imm_i(instruction);
                switch (funct3) {
                    case 0x0:
                        cpu->regs[rd] = cpu->regs[rs1] + (uint32_t)imm_i;
                        break;
                    case 0x1:
                        if (funct7 == 0x00U) {
                            cpu->regs[rd] = cpu->regs[rs1] << (rs2 & 0x1fU);
                        } else {
                            raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                            advance_pc = 0;
                        }
                        break;
                    case 0x2:
                        cpu->regs[rd] = ((int32_t)cpu->regs[rs1] < imm_i) ? 1U : 0U;
                        break;
                    case 0x3:
                        cpu->regs[rd] = (cpu->regs[rs1] < (uint32_t)imm_i) ? 1U : 0U;
                        break;
                    case 0x4:
                        cpu->regs[rd] = cpu->regs[rs1] ^ (uint32_t)imm_i;
                        break;
                    case 0x5:
                        if (funct7 == 0x00U) {
                            cpu->regs[rd] = cpu->regs[rs1] >> (rs2 & 0x1fU);
                        } else if (funct7 == 0x20U) {
                            cpu->regs[rd] = (uint32_t)((int32_t)cpu->regs[rs1] >> (rs2 & 0x1fU));
                        } else {
                            raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                            advance_pc = 0;
                        }
                        break;
                    case 0x6:
                        cpu->regs[rd] = cpu->regs[rs1] | (uint32_t)imm_i;
                        break;
                    case 0x7:
                        cpu->regs[rd] = cpu->regs[rs1] & (uint32_t)imm_i;
                        break;
                    default:
                        raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                        advance_pc = 0;
                        break;
                }
                break;
            }

            case 0x03: {
                int32_t imm_i = rv32i_imm_i(instruction);
                uint32_t addr = cpu->regs[rs1] + (uint32_t)imm_i;
                switch (funct3) {
                    case 0x0: {
                        uint8_t value8;
                        rc = rv32i_mem_read8(mem, addr, &value8);
                        if (rc != RV32I_MEM_OK) {
                            raise_trap(cpu, map_load_error(rc));
                            advance_pc = 0;
                        } else {
                            cpu->regs[rd] = (uint32_t)(int32_t)(int8_t)value8;
                        }
                        break;
                    }
                    case 0x1: {
                        uint16_t value16;
                        rc = rv32i_mem_read16(mem, addr, &value16);
                        if (rc != RV32I_MEM_OK) {
                            raise_trap(cpu, map_load_error(rc));
                            advance_pc = 0;
                        } else {
                            cpu->regs[rd] = (uint32_t)(int32_t)(int16_t)value16;
                        }
                        break;
                    }
                    case 0x2: {
                        uint32_t value32;
                        rc = rv32i_mem_read32(mem, addr, &value32);
                        if (rc != RV32I_MEM_OK) {
                            raise_trap(cpu, map_load_error(rc));
                            advance_pc = 0;
                        } else {
                            cpu->regs[rd] = value32;
                        }
                        break;
                    }
                    case 0x4: {
                        uint8_t value8;
                        rc = rv32i_mem_read8(mem, addr, &value8);
                        if (rc != RV32I_MEM_OK) {
                            raise_trap(cpu, map_load_error(rc));
                            advance_pc = 0;
                        } else {
                            cpu->regs[rd] = value8;
                        }
                        break;
                    }
                    case 0x5: {
                        uint16_t value16;
                        rc = rv32i_mem_read16(mem, addr, &value16);
                        if (rc != RV32I_MEM_OK) {
                            raise_trap(cpu, map_load_error(rc));
                            advance_pc = 0;
                        } else {
                            cpu->regs[rd] = value16;
                        }
                        break;
                    }
                    default:
                        raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                        advance_pc = 0;
                        break;
                }
                break;
            }

            case 0x23: {
                int32_t imm_s = rv32i_imm_s(instruction);
                uint32_t addr = cpu->regs[rs1] + (uint32_t)imm_s;
                switch (funct3) {
                    case 0x0:
                        rc = rv32i_mem_write8(mem, addr, (uint8_t)(cpu->regs[rs2] & 0xffU));
                        if (rc != RV32I_MEM_OK) {
                            raise_trap(cpu, map_store_error(rc));
                            advance_pc = 0;
                        }
                        break;
                    case 0x1:
                        rc = rv32i_mem_write16(mem, addr, (uint16_t)(cpu->regs[rs2] & 0xffffU));
                        if (rc != RV32I_MEM_OK) {
                            raise_trap(cpu, map_store_error(rc));
                            advance_pc = 0;
                        }
                        break;
                    case 0x2:
                        rc = rv32i_mem_write32(mem, addr, cpu->regs[rs2]);
                        if (rc != RV32I_MEM_OK) {
                            raise_trap(cpu, map_store_error(rc));
                            advance_pc = 0;
                        }
                        break;
                    default:
                        raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                        advance_pc = 0;
                        break;
                }
                break;
            }

            case 0x63: {
                int take_branch = 0;
                int32_t imm_b = rv32i_imm_b(instruction);
                switch (funct3) {
                    case 0x0:
                        take_branch = (cpu->regs[rs1] == cpu->regs[rs2]);
                        break;
                    case 0x1:
                        take_branch = (cpu->regs[rs1] != cpu->regs[rs2]);
                        break;
                    case 0x4:
                        take_branch = ((int32_t)cpu->regs[rs1] < (int32_t)cpu->regs[rs2]);
                        break;
                    case 0x5:
                        take_branch = ((int32_t)cpu->regs[rs1] >= (int32_t)cpu->regs[rs2]);
                        break;
                    case 0x6:
                        take_branch = (cpu->regs[rs1] < cpu->regs[rs2]);
                        break;
                    case 0x7:
                        take_branch = (cpu->regs[rs1] >= cpu->regs[rs2]);
                        break;
                    default:
                        raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                        advance_pc = 0;
                        break;
                }
                if (!cpu->halted && take_branch) {
                    next_pc = cpu->pc + (uint32_t)imm_b;
                }
                break;
            }

            case 0x37:
                cpu->regs[rd] = (uint32_t)rv32i_imm_u(instruction);
                break;

            case 0x17:
                cpu->regs[rd] = cpu->pc + (uint32_t)rv32i_imm_u(instruction);
                break;

            case 0x6f:
                cpu->regs[rd] = cpu->pc + 4U;
                next_pc = cpu->pc + (uint32_t)rv32i_imm_j(instruction);
                break;

            case 0x67:
                if (funct3 != 0x0U) {
                    raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                    advance_pc = 0;
                } else {
                    uint32_t target = (cpu->regs[rs1] + (uint32_t)rv32i_imm_i(instruction)) & ~1U;
                    cpu->regs[rd] = cpu->pc + 4U;
                    next_pc = target;
                }
                break;

            case 0x0f:
                /* FENCE and FENCE.I (Zifencei). There's one hart and no caches, and
                   every fetch reads memory fresh anyway, so neither one has
                   anything to wait on. */
                if (funct3 > 0x1U) {
                    raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                    advance_pc = 0;
                }
                break;

            case 0x73:
                if (funct3 != 0x0U) {
                    raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                    advance_pc = 0;
                } else {
                    uint32_t imm12 = instruction >> 20;
                    if (imm12 == 0x000U && rs1 == 0U && rd == 0U) {
                        if (cpu->regs[17] == 93U) {
                            cpu->exit_code = (int32_t)cpu->regs[10];
                            cpu->halted = 1;
                            cpu->trap_code = RV32I_TRAP_NONE;
                        } else {
                            raise_trap(cpu, RV32I_TRAP_UNSUPPORTED_ECALL);
                            advance_pc = 0;
                        }
                    } else if (imm12 == 0x001U && rs1 == 0U && rd == 0U) {
                        raise_trap(cpu, RV32I_TRAP_BREAKPOINT);
                        advance_pc = 0;
                    } else {
                        raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                        advance_pc = 0;
                    }
                }
                break;

            default:
                raise_trap(cpu, RV32I_TRAP_ILLEGAL_INSTRUCTION);
                advance_pc = 0;
                break;
        }

        cpu->regs[0] = 0U;
        if (advance_pc) {
            cpu->pc = next_pc;
        }
        cpu->steps += 1U;
    }

    result.halted = cpu->halted;
    result.exit_code = cpu->exit_code;
    result.trap_code = cpu->trap_code;
    result.steps = cpu->steps;
    return result;
}
