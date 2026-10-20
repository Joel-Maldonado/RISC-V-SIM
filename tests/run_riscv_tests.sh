#!/usr/bin/env bash
# Builds every rv32ui test from a riscv-tests checkout and runs it on the sim.
# usage: tests/run_riscv_tests.sh path/to/riscv-tests
set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TESTS_DIR="${1:?usage: $0 path/to/riscv-tests}"
SIM="$ROOT_DIR/build/rv32i-sim"
OUT_DIR="$ROOT_DIR/build/riscv-tests"
RISCV_GCC="${RISCV_GCC:-riscv64-elf-gcc}"

# fence.i (Zifencei) and CSRs (Zicsr) aren't part of RV32I, but the assembler
# still has to accept them or the tests that mention them won't even build.
CFLAGS="-march=rv32i_zicsr_zifencei -mabi=ilp32 -nostdlib -nostartfiles \
  -I$ROOT_DIR/tests/riscv-tests -I$TESTS_DIR/isa/macros/scalar \
  -T $ROOT_DIR/programs/linker.ld -Wl,--build-id=none -Wl,--no-warn-rwx-segments"

make -C "$ROOT_DIR" sim >/dev/null || exit 1
mkdir -p "$OUT_DIR"

tests=$(sed -n '/^rv32ui_sc_tests = /,/^$/p' "$TESTS_DIR/isa/rv32ui/Makefrag" |
        tr -d '\\' | sed 's/rv32ui_sc_tests = //')
# ma_data checks that misaligned loads and stores work. Mine trap on purpose
# (MISALIGNED_ACCESS), which the spec allows, so just skip it.
skip="ma_data"

pass=0
fail=0
for t in $tests; do
    if [[ " $skip " == *" $t "* ]]; then
        printf '%-10s skipped\n' "$t"
        continue
    fi
    elf="$OUT_DIR/$t.elf"
    # shellcheck disable=SC2086
    if ! "$RISCV_GCC" $CFLAGS "$TESTS_DIR/isa/rv32ui/$t.S" -o "$elf"; then
        printf '%-10s BUILD FAILED\n' "$t"
        fail=$((fail + 1))
        continue
    fi
    out="$("$SIM" "$elf")"
    if grep -q "HALT_REASON: ECALL_EXIT" <<<"$out" && grep -q "EXIT_CODE: 0$" <<<"$out"; then
        pass=$((pass + 1))
    else
        printf '%-10s FAIL  %s, %s\n' "$t" \
            "$(grep HALT_REASON <<<"$out")" "$(grep EXIT_CODE <<<"$out")"
        fail=$((fail + 1))
    fi
done
echo "rv32ui: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
