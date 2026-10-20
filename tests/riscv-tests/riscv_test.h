// Just enough of the riscv-tests environment to run the rv32ui tests here.
// The real ones set up CSRs, trap vectors and a tohost mailbox, and I don't
// have any of that. So a test just exits with ECALL (a7 = 93) like any other
// program, with exit code 0 if it passed or the failing test's number if not.
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
