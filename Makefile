PEBBLE_TOOLCHAIN_PATH ?= /usr/local/Cellar/pebble-toolchain/2.0/arm-cs-tools/bin

ifneq "$(WAIT_GDB)" ""
QEMUFLAGS += -S
endif

PFX ?= $(PEBBLE_TOOLCHAIN_PATH)/arm-none-eabi-
CC = $(PFX)gcc

ASFLAGS += -ggdb
CFLAGS += -g

CFLAGS += -mcpu=cortex-m3 -mthumb -Icmsis

OBJS = boot.o main.o

target: fw.qemu_flash.bin

qemu: fw.qemu_flash.bin
	qemu-pebble -rtc base=localtime -serial null -serial null -serial stdio -gdb tcp::63770,server -machine pebble-bb2 -cpu cortex-m3 -pflash fw.qemu_flash.bin $(QEMUFLAGS)

gdb:
	$(PFX)gdb -ex 'target remote localhost:63770' -ex 'sym fw.elf'

fw.elf: $(OBJS) fw.lds
	$(PFX)ld -o $@ -T fw.lds $(OBJS) 

%.bin: %.elf
	$(PFX)objcopy $< -O binary $@

%.qemu_flash.bin: %.bin tintin_boot.bin
	cat tintin_boot.bin $< > $@
	
.PRECIOUS: fw.elf fw.bin fw.qemu_flash.bin