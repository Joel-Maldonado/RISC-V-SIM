// Just enough of the riscv-tests environment to run the rv32ui tests on this
// simulator. The real environments set up CSRs, trap vectors and a tohost
// mailbox; we have none of those, so a test exits with ECALL (a7 = 93) like
// any other program: exit code 0 on pass, the failing test's number on fail.
#ifndef RV32I_SIM_RISCV_TEST_H
#define RV32I_SIM_RISCV_TEST_H

#define RVTEST_RV32U
#define RVTEST_RV64U
#define TESTNUM gp

#define RVTEST_CODE_BEGIN \
        .section .text.start; \
        .globl _start; \
_start: \
        li TESTNUM, 0;

#define RVTEST_CODE_END unimp

#define RVTEST_PASS li a0, 0; li a7, 93; ecall
#define RVTEST_FAIL mv a0, TESTNUM; li a7, 93; ecall

#define RVTEST_DATA_BEGIN .align 4; .global begin_signature; begin_signature:
#define RVTEST_DATA_END .align 4; .global end_signature; end_signature:

#endif
