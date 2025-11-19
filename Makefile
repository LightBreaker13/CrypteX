TARGET := kernel.bin
ISO_DIR := iso
BUILD := build

CC := gcc
LD := ld
AS := as

CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -nostdlib -m32 -I./src
ASFLAGS := --32
LDFLAGS := -m elf_i386

SRCS := \
  src/boot.s \
  src/kernel.c \
  src/fb.c \
  src/font8x16.c \
  src/input.c \
  src/gui.c \
  src/shell.c \
  src/sysmon.c \
  src/process.c \
  src/profiles.c \
  src/ledger.c \
  src/crypto.c \
  src/fs.c \
  src/raid.c \
  src/compat_win.c \
  src/anim.c \
  src/audio.c \
  src/console.c \
  src/installer.c \
  src/storage_detect.c

OBJS := $(SRCS:%.c=$(BUILD)/%.o)
OBJS := $(OBJS:%.s=$(BUILD)/%.o)

.PHONY: all clean iso run

all: iso

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/$(TARGET): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -T linker.ld -o $@ $(OBJS)

iso: $(BUILD)/$(TARGET) grub/grub.cfg
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(BUILD)/$(TARGET) $(ISO_DIR)/boot/kernel.bin
	cp grub/grub.cfg $(ISO_DIR)/boot/grub/
	grub-mkrescue -o myos.iso $(ISO_DIR)

run: iso
	qemu-system-i386 -cdrom myos.iso -m 512 -display sdl

clean:
	rm -rf $(BUILD) $(ISO_DIR) myos.iso

