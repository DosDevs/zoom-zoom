all: Elfo

BASE_ADDRESS=0xfffffffff1000000  # Second 16 MiB of top 256 MiB.
# TEXT_ADDRESS=0x300b0
# DATA_ADDRESS=0x36000
# BSS_ADDRESS=0x37000
include ../Makefile.kern

Elfo: Entry.o Bootstrap.o Map.o Elf64.o
	$(LINK) $^ -o $@ ../Common/libcommon.a
	cp $@ ../fd/

