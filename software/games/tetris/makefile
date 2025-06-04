#
# B68K Computer - Copyright (c) 2024 Lougous
# https://github.com/Lougous/b68k
#

XCROOT=m68k-linux-gnu-

AS=${XCROOT}gcc
ASFLAGS=-march=68000 -c -x assembler-with-cpp

CC=${XCROOT}gcc
CFLAGS=-Wall -m68000 -mstrict-align -O1 -nostdlib \
-fdata-sections -ffunction-sections \
-I../../system/lib/libc/include -Iinclude -I../../system/include -I. -c

LD=${XCROOT}ld
LDFLAGS=-T tetris.ld -nostdlib -Wl,--build-id=none -Wl,--gc-sections -L../../system/lib/libc
LDLIBS=-lc

BIN=tetris

# entries
all:	${BIN}

tetris:	crt0.o cache.o vgm.o tetris.o 

clean:
	rm -f *.o ${BIN}

