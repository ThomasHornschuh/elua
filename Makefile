TARGETDIR ?= ~/upload
BOARD ?=  bonfire_papilio_pro
TARGET ?= lua

.PHONY: build clean
build:
	lua5.1 build_elua.lua board=$(BOARD) target=$(TARGET)
	riscv32-unknown-elf-objdump -S -d elua_$(TARGET)_$(BOARD).elf  >$(TARGET)_$(BOARD).lst

clean:
	lua5.1 build_elua.lua -c board=$(BOARD) target=$(TARGET)


%.img:%.bin
	lua5.1 boot_image.lua $*

%.bin : elua_$(TARGET)_$(BOARD).elf
		riscv32-unknown-elf-objcopy -S -O binary $<  $@


bin : $(TARGETDIR)/elua_$(TARGET)_$(BOARD).bin

img: elua_$(TARGET)_$(BOARD).img

all:
	$(MAKE) build
	$(MAKE) bin
	$(MAKE) img

