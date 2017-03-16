TARGETDIR ?= ~/upload
BOARD ?=  bonfire_papilio_pro
TARGET ?= lualong

.PHONY: build
build:
	lua5.1 build_elua.lua board=$(BOARD) target=$(TARGET)
	riscv32-unknown-elf-objdump -S -d elua_$(TARGET)_$(BOARD).elf  >$(TARGET)_$(BOARD).lst
    
%.bin : elua_$(TARGET)_$(BOARD).elf
		riscv32-unknown-elf-objcopy -S -O binary $<  $@
		

bin : $(TARGETDIR)/elua_$(TARGET)_$(BOARD).bin
		

all:
	$(MAKE) build
	$(MAKE) bin
		
