#
# B68K Computer - Copyright (c) 2024 Lougous
# https://github.com/Lougous/b68k
#
# System/binaries - makefile
#

XCROOT=m68k-linux-gnu-

AS=${XCROOT}gcc
ASFLAGS=-march=68000 -c -x assembler-with-cpp -I../include

CC=${XCROOT}gcc
CFLAGS=-Wall -m68000 -mstrict-align -fno-builtin -O1 -nostdlib \
-fdata-sections -ffunction-sections \
-I../lib/libc/include -Iinclude -I../include -I.

LD=${XCROOT}ld
LDFLAGS=-T crt0.ld -nostdlib -Wl,--build-id=none -Wl,--gc-sections -L../lib/libc -L/usr/lib/gcc-cross/m68k-linux-gnu/9
LDLIBS=-lc -lgcc

OBJCOPY=${XCROOT}objcopy
OBJDUMP=${XCROOT}objdump

BIN=ls sh cat init test mount

# entries
all:	libc ${BIN}
	mv ${BIN} build/

libc:
	make -C ../lib/libc

ls:	ls.o crt0.o
sh:	sh.o crt0.o
cat:	cat.o crt0.o
init:	init.o crt0.o
test:	test.o crt0.o
mount:	mount.o crt0.o

clean:
	make -C ../lib/libc clean
	rm -f *.o build/*

