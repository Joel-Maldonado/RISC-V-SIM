#ifndef RV32I_DECODE_H
#define RV32I_DECODE_H

#include <stdint.h>

uint32_t rv32i_get_opcode(uint32_t instruction);
uint32_t rv32i_get_rd(uint32_t instruction);
uint32_t rv32i_get_funct3(uint32_t instruction);
uint32_t rv32i_get_rs1(uint32_t instruction);
uint32_t rv32i_get_rs2(uint32_t instruction);
uint32_t rv32i_get_funct7(uint32_t instruction);

int32_t rv32i_imm_i(uint32_t instruction);
int32_t rv32i_imm_s(uint32_t instruction);
int32_t rv32i_imm_b(uint32_t instruction);
int32_t rv32i_imm_u(uint32_t instruction);
int32_t rv32i_imm_j(uint32_t instruction);

const char *rv32i_mnemonic(uint32_t instruction);

#endif
