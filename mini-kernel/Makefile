ifeq ($(origin CC),default)
CC := clang
endif
HOST_CC ?= clang
LLVM_PREFIX ?= /opt/homebrew/opt/llvm/bin
LD_LLD ?= $(shell command -v ld.lld 2>/dev/null || echo ld.lld)
OBJCOPY ?= $(shell command -v llvm-objcopy 2>/dev/null || { test -x $(LLVM_PREFIX)/llvm-objcopy && echo $(LLVM_PREFIX)/llvm-objcopy; } || echo llvm-objcopy)
QEMU ?= qemu-system-x86_64

BUILD_DIR := build
KERNEL_ELF := $(BUILD_DIR)/mini-kernel.elf
BOOTSECTOR_BIN := $(BUILD_DIR)/bootsector.bin
STAGE2_RAW := $(BUILD_DIR)/stage2.raw
STAGE2_BIN := $(BUILD_DIR)/stage2.bin
DISK_IMG := $(BUILD_DIR)/mini-kernel.img

KERNEL_CFLAGS := -target x86_64-elf -std=c11 -ffreestanding -fno-stack-protector \
	-fno-pic -mno-red-zone -O2 -Wall -Wextra -Werror \
	-Ikernel/include

HOST_CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -Ikernel/include -DKERNEL_HOST_TEST

KERNEL_SRCS := \
	kernel/main.c \
	kernel/panic.c \
	kernel/lib/mem.c \
	kernel/lib/string.c \
	kernel/security/assert.c \
	kernel/arch/x86_64/cpu.c \
	kernel/arch/x86_64/serial.c

KERNEL_OBJS := $(KERNEL_SRCS:%.c=$(BUILD_DIR)/%.o) $(BUILD_DIR)/boot/multiboot2.o

HOST_TEST_SRCS := \
	tests/test_kernel_lib.c \
	kernel/lib/mem.c \
	kernel/lib/string.c \
	kernel/security/assert.c

.PHONY: all check clean image kernel-objects run run-bootsector run-image toolchain \
	validate-bootsector validate-stage2

all: check image kernel-objects

toolchain:
	@command -v $(CC) >/dev/null || { echo "missing compiler: $(CC)"; exit 1; }
	@command -v $(LD_LLD) >/dev/null || { echo "missing ELF linker: $(LD_LLD)"; exit 1; }

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/boot/%.o: boot/%.S | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) -target x86_64-elf -c $< -o $@

$(BUILD_DIR)/bootsector.o: boot/bootsector.S $(STAGE2_BIN) | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) -target i386-unknown-elf -DSTAGE2_SECTORS=$(shell expr \( `wc -c < $(STAGE2_BIN) | tr -d ' '` + 511 \) / 512) -c $< -o $@

$(BOOTSECTOR_BIN): $(BUILD_DIR)/bootsector.o
	@command -v $(OBJCOPY) >/dev/null || { echo "missing objcopy: $(OBJCOPY)"; exit 1; }
	$(OBJCOPY) -O binary -j .text $< $@
	$(MAKE) validate-bootsector

$(BUILD_DIR)/stage2.o: boot/stage2.S | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) -target i386-unknown-elf -c $< -o $@

$(STAGE2_RAW): $(BUILD_DIR)/stage2.o
	@command -v $(OBJCOPY) >/dev/null || { echo "missing objcopy: $(OBJCOPY)"; exit 1; }
	$(OBJCOPY) -O binary -j .text $< $@

$(STAGE2_BIN): $(STAGE2_RAW)
	@bytes=$$(wc -c < $(STAGE2_RAW) | tr -d ' '); \
	padded=$$(( ((bytes + 511) / 512) * 512 )); \
	cp $(STAGE2_RAW) $@; \
	truncate -s $$padded $@
	$(MAKE) validate-stage2

$(DISK_IMG): $(BOOTSECTOR_BIN) $(STAGE2_BIN)
	cat $(BOOTSECTOR_BIN) $(STAGE2_BIN) > $@
	@test "$$(wc -c < $@ | tr -d ' ')" = "$$(($$(wc -c < $(BOOTSECTOR_BIN) | tr -d ' ') + $$(wc -c < $(STAGE2_BIN) | tr -d ' ')))" || { echo "disk image size mismatch"; exit 1; }
	@echo "disk image built: $@"

image: $(DISK_IMG)

$(KERNEL_ELF): toolchain $(KERNEL_OBJS) boot/linker.ld
	$(LD_LLD) -nostdlib -z max-page-size=0x1000 -T boot/linker.ld -o $@ $(KERNEL_OBJS)

kernel-objects: $(KERNEL_OBJS)
	@echo "kernel objects compiled"

$(BUILD_DIR)/host-tests: $(HOST_TEST_SRCS) | $(BUILD_DIR)
	$(HOST_CC) $(HOST_CFLAGS) $(HOST_TEST_SRCS) -o $@

check: $(BUILD_DIR)/host-tests
	$(BUILD_DIR)/host-tests
	@echo "host checks passed"

validate-bootsector:
	@test -f $(BOOTSECTOR_BIN) || { echo "missing $(BOOTSECTOR_BIN)"; exit 1; }
	@test "$$(wc -c < $(BOOTSECTOR_BIN) | tr -d ' ')" = "512" || { echo "bootsector must be exactly 512 bytes"; exit 1; }
	@tail -c 2 $(BOOTSECTOR_BIN) | od -An -tx1 | tr -s ' ' | grep -q '55 aa' || { echo "bootsector signature missing"; exit 1; }
	@echo "bootsector validated"

validate-stage2:
	@test -f $(STAGE2_BIN) || { echo "missing $(STAGE2_BIN)"; exit 1; }
	@test "$$(( $$(wc -c < $(STAGE2_BIN) | tr -d ' ') % 512 ))" = "0" || { echo "stage2 must be sector aligned"; exit 1; }
	@test "$$(wc -c < $(STAGE2_BIN) | tr -d ' ')" -le "8704" || { echo "stage2 currently must fit in one 18-sector floppy track"; exit 1; }
	@echo "stage2 validated"

run: $(KERNEL_ELF)
	@command -v $(QEMU) >/dev/null || { echo "missing emulator: $(QEMU)"; exit 1; }
	$(QEMU) -kernel $(KERNEL_ELF) -serial stdio -display none -no-reboot -no-shutdown

run-bootsector: $(BOOTSECTOR_BIN)
	@command -v $(QEMU) >/dev/null || { echo "missing emulator: $(QEMU)"; exit 1; }
	$(QEMU) -drive format=raw,file=$(BOOTSECTOR_BIN) -serial stdio -display curses -no-reboot

run-image: $(DISK_IMG)
	@command -v $(QEMU) >/dev/null || { echo "missing emulator: $(QEMU)"; exit 1; }
	$(QEMU) -drive format=raw,file=$(DISK_IMG) -serial stdio -display curses -no-reboot

clean:
	rm -rf $(BUILD_DIR)
