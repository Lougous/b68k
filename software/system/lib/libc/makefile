#
# B68K Computer - Copyright (c) 2024 Lougous
# https://github.com/Lougous/b68k
#
# System/libc - makefile
#

XCROOT=m68k-linux-gnu-

AS=${XCROOT}gcc
ASFLAGS=-march=68000 -c -x assembler-with-cpp

CC=${XCROOT}gcc
CFLAGS=-Wall -m68000 -mstrict-align -fno-builtin -O1 -nostdlib \
-fdata-sections -ffunction-sections \
-Iinclude -I../../include -I. -c

LD=${XCROOT}ld
LDFLAGS=-nostdlib -L../lib/libc -L/usr/lib/gcc-cross/m68k-linux-gnu/9 -lc -lgcc

OBJCOPY=${XCROOT}objcopy
OBJDUMP=${XCROOT}objdump

# object files list
OBJ=\
src/__libc.o	\
src/trap.o	\
src/wait.o	\
src/signal.o	\
src/unistd.o	\
src/ioctl.o	\
src/string.o	\
src/stdio.o	\
src/stdlib.o	\
src/getdents.o

# entries
libc.a: ${OBJ}
	${AR} -rv $@ ${OBJ}

clean:
	rm -f libc.a ${OBJ}
