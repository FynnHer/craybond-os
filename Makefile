
sname := $(shell uname -s)

CROSS_PREFIX=aarch64-none-elf-

all: os.elf

kernel.o: kernel.c
	$(CROSS_PREFIX)gcc -c $< -o $@

boot.o: boot.s
	$(CROSS_PREFIX)as -c $< -o $@

os.elf: kernel.o boot.o 
	$(CROSS_PREFIX)ld -T linker.ld $^ -o $@

os.bin: os.elf 
	$(CROSS_PREFIX)objcopy -O binary $< $@

clean:
	rm -f os.bin os.elf boot.o kernel.o
