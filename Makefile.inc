ASM=yasm
CC=clang++50 -c -std=c++17
LINKER=ld -dynamic-linker /libexec/ld-elf.so.1 -L/usr/lib

clean:
	rm -fv *.o

%.o: %.cpp
	$(COMPILE) $< -o $@

%.o: %.asm
	$(ASM) -f elf -m amd64 -o $@ $<

.PHONY: all clean
