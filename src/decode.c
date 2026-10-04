#include "decode.h"

static int32_t sign_extend(uint32_t value, unsigned bits) {
    uint32_t mask = 1U << (bits - 1U);
    return (int32_t)((value ^ mask) - mask);
}

uint32_t rv32i_get_opcode(uint32_t instruction) {
    return instruction & 0x7fU;
}

uint32_t rv32i_get_rd(uint32_t instruction) {
    return (instruction >> 7) & 0x1fU;
}

uint32_t rv32i_get_funct3(uint32_t instruction) {
    return (instruction >> 12) & 0x7U;
}

uint32_t rv32i_get_rs1(uint32_t instruction) {
    return (instruction >> 15) & 0x1fU;
}

uint32_t rv32i_get_rs2(uint32_t instruction) {
    return (instruction >> 20) & 0x1fU;
}

uint32_t rv32i_get_funct7(uint32_t instruction) {
    return (instruction >> 25) & 0x7fU;
}

int32_t rv32i_imm_i(uint32_t instruction) {
    return sign_extend(instruction >> 20, 12U);
}

int32_t rv32i_imm_s(uint32_t instruction) {
    uint32_t imm = ((instruction >> 25) << 5) | ((instruction >> 7) & 0x1fU);
    return sign_extend(imm, 12U);
}

int32_t rv32i_imm_b(uint32_t instruction) {
    uint32_t imm = (((instruction >> 31) & 0x1U) << 12) |
                   (((instruction >> 7) & 0x1U) << 11) |
                   (((instruction >> 25) & 0x3fU) << 5) |
                   (((instruction >> 8) & 0xfU) << 1);
    return sign_extend(imm, 13U);
}

int32_t rv32i_imm_u(uint32_t instruction) {
    return (int32_t)(instruction & 0xfffff000U);
}

int32_t rv32i_imm_j(uint32_t instruction) {
    uint32_t imm = (((instruction >> 31) & 0x1U) << 20) |
                   (((instruction >> 12) & 0xffU) << 12) |
                   (((instruction >> 20) & 0x1U) << 11) |
                   (((instruction >> 21) & 0x3ffU) << 1);
    return sign_extend(imm, 21U);
}

const char *rv32i_mnemonic(uint32_t instruction) {
    uint32_t opcode = rv32i_get_opcode(instruction);
    uint32_t funct3 = rv32i_get_funct3(instruction);
    uint32_t funct7 = rv32i_get_funct7(instruction);
    uint32_t imm12 = instruction >> 20;

    switch (opcode) {
        case 0x33:
            switch (funct3) {
                case 0x0:
                    return (funct7 == 0x20U) ? "sub" : (funct7 == 0x00U ? "add" : "illegal");
                case 0x1:
                    return (funct7 == 0x00U) ? "sll" : "illegal";
                case 0x2:
                    return (funct7 == 0x00U) ? "slt" : "illegal";
                case 0x3:
                    return (funct7 == 0x00U) ? "sltu" : "illegal";
                case 0x4:
                    return (funct7 == 0x00U) ? "xor" : "illegal";
                case 0x5:
                    return (funct7 == 0x20U) ? "sra" : (funct7 == 0x00U ? "srl" : "illegal");
                case 0x6:
                    return (funct7 == 0x00U) ? "or" : "illegal";
                case 0x7:
                    return (funct7 == 0x00U) ? "and" : "illegal";
                default:
                    return "illegal";
            }
        case 0x13:
            switch (funct3) {
                case 0x0:
                    return "addi";
                case 0x1:
                    return (funct7 == 0x00U) ? "slli" : "illegal";
                case 0x2:
                    return "slti";
                case 0x3:
                    return "sltiu";
                case 0x4:
                    return "xori";
                case 0x5:
                    return (funct7 == 0x20U) ? "srai" : (funct7 == 0x00U ? "srli" : "illegal");
                case 0x6:
                    return "ori";
                case 0x7:
                    return "andi";
                default:
                    return "illegal";
            }
        case 0x03:
            switch (funct3) {
                case 0x0:
                    return "lb";
                case 0x1:
                    return "lh";
                case 0x2:
                    return "lw";
                case 0x4:
                    return "lbu";
                case 0x5:
                    return "lhu";
                default:
                    return "illegal";
            }
        case 0x23:
            switch (funct3) {
                case 0x0:
                    return "sb";
                case 0x1:
                    return "sh";
                case 0x2:
                    return "sw";
                default:
                    return "illegal";
            }
        case 0x63:
            switch (funct3) {
                case 0x0:
                    return "beq";
                case 0x1:
                    return "bne";
                case 0x4:
                    return "blt";
                case 0x5:
                    return "bge";
                case 0x6:
                    return "bltu";
                case 0x7:
                    return "bgeu";
                default:
                    return "illegal";
            }
        case 0x37:
            return "lui";
        case 0x17:
            return "auipc";
        case 0x6f:
            return "jal";
        case 0x67:
            return (funct3 == 0x0U) ? "jalr" : "illegal";
        case 0x0f:
            return (funct3 == 0x0U) ? "fence" : (funct3 == 0x1U ? "fence.i" : "illegal");
        case 0x73:
            if (funct3 != 0x0U) {
                return "illegal";
            }
            if (imm12 == 0x000U) {
                return "ecall";
            }
            if (imm12 == 0x001U) {
                return "ebreak";
            }
            return "system";
        default:
            return "unknown";
    }
}
