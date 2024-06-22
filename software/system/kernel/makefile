# tools and flags definitions
XCROOT=m68k-linux-gnu-

AS=${XCROOT}gcc
ASFLAGS=-march=68000 -c -x assembler-with-cpp

CC=${XCROOT}gcc
CFLAGS=-Wall -mcpu=68000 -mstrict-align -O1 -nostdlib -fno-builtin-fprintf \
-Ilibc/include -Iinclude -I../include -I.

LD=${XCROOT}gcc
LDFLAGS=-Wl,--build-id=none -mcpu=68000 -nostdlib

OBJCOPY=${XCROOT}objcopy
OBJDUMP=${XCROOT}objdump

# object files list
# !!! DO NOT CHANGE ORDER FOR FIRST OBJECT FILE !!!
OBJ=\
srt0.o \
irq.o \
scall.o \
proc.o \
dev.o \
mem.o \
msg.o \
system.o \
tty.o \
mio.o \
mouse.o \
av_pcm.o \
av_opl2.o \
clock.o \
vfs.o \
fs/fat.o \
fs/devfs.o \
fs/rfs.o \
libc/src/stdio.o libc/src/stdlib.o libc/src/string.o \
dsh/ls.o dsh/cat.o dsh/load.o dsh/md.o dsh/mm.o dsh/ramtest.o dsh/readx.o dsh/dsh.o

# entries
system.elf: system.ld ${OBJ}
	${LD} -o $@ ${LDFLAGS} -T $^ -lgcc
	${OBJCOPY} -O binary --only-section=.text --only-section=.rodata --only-section=.sdata $@ system-img.bin
	echo "const unsigned long system_bin[] = {\n" > system-img.c
	cat system.bin | hexdump -v -e '1/4 "0x%x," "\n"' >> system-img.c
	echo "};\n" >> system-img.c
	${OBJDUMP} -d $@ > system.debug

tty.c:	flex-img.c

flex-img.c: ../../../boards/b68k-av/flex/flex.rbf
	xxd -i $^ | sed 's/unsigned/const unsigned/' > flex-img.c

clean:
	rm -f system.elf system.bin ${OBJ}
