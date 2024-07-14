#
# B68K Computer - Copyright (c) 2024 Lougous
# https://github.com/Lougous/b68k
#

XCROOT=m68k-linux-gnu-

AS=${XCROOT}gcc
ASFLAGS=-march=68000 -c -x assembler-with-cpp

CC=${XCROOT}gcc
CFLAGS=-Wall -m68000 -mstrict-align -O1 -nostdlib \
-I../../system/lib/libc/include -Iinclude -I../../system/include -I. -c

LD=${XCROOT}ld
LDFLAGS=-T msweep.ld -nostdlib -Wl,--build-id=none -L../../system/lib/libc
LDLIBS=-lc

OBJCOPY=${XCROOT}objcopy
OBJDUMP=${XCROOT}objdump

BIN=msweep

# entries
all:	${BIN}

msweep:	crt0.o msweep.o 

clean:
	rm -f *.o ${BIN}

