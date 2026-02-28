SHELL := /bin/sh

CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
CPPFLAGS ?=
LDFLAGS ?=
INCLUDES := -Iinclude

SRC := src/main.c src/cpu.c src/decode.c src/memory.c src/elf_loader.c
OBJ := $(SRC:src/%.c=build/%.o)
SIM := build/rv32i-sim

RISCV_PREFIX ?= riscv64-elf
RISCV_GCC ?= $(RISCV_PREFIX)-gcc
RISCV_OBJDUMP ?= $(RISCV_PREFIX)-objdump
RISCV_CFLAGS ?= -march=rv32i -mabi=ilp32 -ffreestanding -nostdlib -fno-builtin -O2 -Wall -Wextra
RISCV_LDFLAGS ?= -T programs/linker.ld -Wl,--build-id=none

DEMO_DIR := build/demo
DEMO_ASM := $(DEMO_DIR)/demo.s
DEMO_ELF := $(DEMO_DIR)/demo.elf
DEMO_DISASM := $(DEMO_DIR)/demo.disasm

.PHONY: all sim toolchain-check demo-asm demo-elf run-demo test clean

all: sim

build:
	@mkdir -p build

$(DEMO_DIR):
	@mkdir -p $(DEMO_DIR)

build/%.o: src/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SIM): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@

sim: $(SIM)

toolchain-check:
	@command -v $(RISCV_GCC) >/dev/null 2>&1 || { echo "Missing $(RISCV_GCC). Install with: brew install riscv64-elf-binutils riscv64-elf-gcc"; exit 1; }
	@command -v $(RISCV_OBJDUMP) >/dev/null 2>&1 || { echo "Missing $(RISCV_OBJDUMP). Install with: brew install riscv64-elf-binutils"; exit 1; }

demo-asm: toolchain-check | $(DEMO_DIR)
	$(RISCV_GCC) $(RISCV_CFLAGS) -S programs/demo.c -o $(DEMO_ASM)

demo-elf: toolchain-check | $(DEMO_DIR)
	$(RISCV_GCC) $(RISCV_CFLAGS) $(RISCV_LDFLAGS) programs/start.S programs/demo.c -o $(DEMO_ELF)
	$(RISCV_OBJDUMP) -d $(DEMO_ELF) > $(DEMO_DISASM)

run-demo: sim demo-elf
	$(SIM) $(DEMO_ELF) --dump-regs

test: sim demo-elf
	@tests/test_sim_demo.sh

clean:
	rm -rf build
