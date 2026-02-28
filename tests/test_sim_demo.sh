#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SIM="$ROOT_DIR/build/rv32i-sim"
DEMO_ELF="$ROOT_DIR/build/demo/demo.elf"

RISCV_GCC="${RISCV_GCC:-riscv64-elf-gcc}"
RISCV_FLAGS="-march=rv32i -mabi=ilp32 -ffreestanding -nostdlib -fno-builtin -O2"
RISCV_LD_FLAGS="-T $ROOT_DIR/programs/linker.ld -Wl,--build-id=none"

cd "$ROOT_DIR"

make sim >/dev/null
make demo-elf >/dev/null

main_output="$($SIM "$DEMO_ELF" --dump-regs)"
printf '%s\n' "$main_output"

grep -q "HALT_REASON: ECALL_EXIT" <<<"$main_output"
grep -q "EXIT_CODE: 42" <<<"$main_output"
grep -Eq "x10 \( *a0\): 0x0000002a" <<<"$main_output"
grep -Eq "x17 \( *a7\): 0x0000005d" <<<"$main_output"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

cat > "$tmp_dir/misaligned.S" <<'ASM'
    .section .text.start
    .globl _start
    .option norvc
_start:
    la sp, __stack_top
    li t0, 1
    lw t1, 0(t0)
    li a0, 0
    li a7, 93
    ecall
ASM

"$RISCV_GCC" $RISCV_FLAGS $RISCV_LD_FLAGS "$tmp_dir/misaligned.S" -o "$tmp_dir/misaligned.elf"
misaligned_output="$($SIM "$tmp_dir/misaligned.elf" || true)"
printf '%s\n' "$misaligned_output"
grep -q "HALT_REASON: MISALIGNED_ACCESS" <<<"$misaligned_output"

cat > "$tmp_dir/illegal.S" <<'ASM'
    .section .text.start
    .globl _start
    .option norvc
_start:
    la sp, __stack_top
    .word 0xffffffff
    li a0, 0
    li a7, 93
    ecall
ASM

"$RISCV_GCC" $RISCV_FLAGS $RISCV_LD_FLAGS "$tmp_dir/illegal.S" -o "$tmp_dir/illegal.elf"
illegal_output="$($SIM "$tmp_dir/illegal.elf" || true)"
printf '%s\n' "$illegal_output"
grep -q "HALT_REASON: ILLEGAL_INSTRUCTION" <<<"$illegal_output"

step_limit_output="$($SIM "$DEMO_ELF" --max-steps 2 || true)"
printf '%s\n' "$step_limit_output"
grep -q "HALT_REASON: STEP_LIMIT" <<<"$step_limit_output"

if "$SIM" "$DEMO_ELF" --max-steps -1 >"$tmp_dir/neg_steps.out" 2>&1; then
    echo "Expected --max-steps -1 to be rejected" >&2
    exit 1
fi
grep -q "Invalid --max-steps value: -1" "$tmp_dir/neg_steps.out"

echo "All simulator checks passed."
