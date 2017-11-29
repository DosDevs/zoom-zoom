all: libcommon.a
include ../Makefile.kern

libcommon.a: Console.o Argument_List.o Stringer.o stdlib.o
	$(AR) -r $@ $^

