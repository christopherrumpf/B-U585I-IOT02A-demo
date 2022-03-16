-include Makefile.local

AARCH64CC = $(CROSSPREFIX)gcc
HOSTCC = $(PREFIX)gcc
HOSTAR = $(PREFIX)ar
CFLAGS = -g -Wall

all: libcoreio.so libcoreio.a vmmio-test-inside vmmio-test-outside

libcoreio.so: corehdl-base.c coreio.c corehdl.h coreio.h
	$(HOSTCC) $(CFLAGS) -shared -fPIC -o $@ corehdl-base.c coreio.c
	@chmod a-x $@
libcoreio.a: corehdl-base.c coreio.c corehdl.h coreio.h
	$(HOSTCC) $(CFLAGS) -c -o corehdl-base.o corehdl-base.c
	$(HOSTCC) $(CFLAGS) -c -o coreio.o coreio.c
	$(HOSTAR) cr $@ corehdl-base.o coreio.o
	@rm -f corehdl-base.o coreio.o

vmmio-test-inside: vmmio-test-inside.c
	$(AARCH64CC) $(CFLAGS) -o$@ $^
vmmio-test-outside: vmmio-test-outside.c libcoreio.a
	$(HOSTCC) $(CFLAGS) -o$@ $^

clean:
	rm -f libcoreio.so libcoreio.a vmmio-test-inside vmmio-test-outside
