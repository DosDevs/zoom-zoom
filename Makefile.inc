ASM=yasm
CC=clang++ -c -std=c++1z
LINKER=ld -dynamic-linker /libexec/ld-elf.so.1 -L/usr/lib

clean:
	rm -fv *.o

%.o: %.cpp
	$(COMPILE) $< -o $@

%.o: %.asm
	$(ASM) -f elf -m amd64 -o $@ $<

.PHONY: all clean
