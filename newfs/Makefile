all: NewFs

include ../Makefile.user

NewFs: main.o Fat_12.o
	$(LINK) $^ -o $@

