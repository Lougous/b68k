#
# B68K Computer - Copyright (c) 2024 Lougous
# https://github.com/Lougous/b68k
#

XCROOT=m68k-linux-gnu-

AS=${XCROOT}gcc
ASFLAGS=-march=68000 -c -x assembler-with-cpp

CC=${XCROOT}gcc
CFLAGS=-Wall -m68000 -mstrict-align -O1 -nostdlib \
-I../system/lib/libc/include -Iinclude -I../system/include -I. -c

LD=${XCROOT}ld
LDFLAGS=-T opl2play.ld -nostdlib -Wl,--build-id=none -L../system/lib/libc -L/usr/lib/gcc-cross/m68k-linux-gnu/9 crt0.o 
LDLIBS=-lc

OBJCOPY=${XCROOT}objcopy
OBJDUMP=${XCROOT}objdump

BIN=opl2play

# entries
all:	crt0.o ${BIN}

opl2play:	opl2play.o

clean:
	rm -f *.o ${BIN}

