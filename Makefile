PREFIX := riscv64-unknown-elf
CC := $(PREFIX)-gcc
OBJCOPY := $(PREFIX)-objcopy
SIZE := $(PREFIX)-size

TARGET := firmware
BUILD := build

ARCHFLAGS := -march=rv32ec_zicsr -mabi=ilp32e

CFLAGS := $(ARCHFLAGS)
CFLAGS += -Wall -Wextra
CFLAGS += -Os -ffreestanding
CFLAGS += -ffunction-sections -fdata-sections

ASFLAGS := $(ARCHFLAGS)

LDFLAGS := $(ARCHFLAGS)
LDFLAGS += -T linker.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -nostdlib

OBJS := $(addprefix $(BUILD)/,startup.o main.o uart.o flash.o crc.o)
ELF := $(BUILD)/$(TARGET).elf
BIN := $(BUILD)/$(TARGET).bin

MINICHLINK := $(HOME)/pico/ch32v003fun/minichlink/minichlink
PROGRAMMER := -C ardulink -c /dev/ttyACM0

.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

all: $(BIN)

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: %.c Makefile | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/startup.o: startup.S Makefile | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(ELF): $(OBJS) linker.ld Makefile
	$(CC) $(LDFLAGS) $(OBJS) -o $@
	$(SIZE) $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

flash: $(BIN)
	$(MINICHLINK) $(PROGRAMMER) -U -w $(BIN) bootloader

clean:
	rm -rf $(BUILD)

.PHONY: all flash clean