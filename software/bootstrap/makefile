# tools and flags definitions
XCROOT=m68k-linux-gnu-

AS=${XCROOT}gcc
ASFLAGS=-march=68000 -c -x assembler-with-cpp

CC=${XCROOT}gcc
CFLAGS=-Wall -mcpu=68000 -mstrict-align -O1 -nostdlib -fno-builtin-fprintf \
-I../system/kernel -I../system/include

LD=${XCROOT}gcc
LDFLAGS=-Wl,--build-id=none -mcpu=68000 -nostdlib
#-L/usr/lib/gcc-cross/m68k-linux-gnu/9

OBJCOPY=${XCROOT}objcopy
OBJDUMP=${XCROOT}objdump

bootstrap-img.c:	boot_sector.bin loader.bin
	cat $^ > bootstrap.bin
	echo "const unsigned long bootstrap_bin[] = {\n" > bootstrap-img.c
	cat bootstrap.bin | hexdump -v -e '1/4 "0x%x," "\n"' >> bootstrap-img.c
	echo "};\n" >> bootstrap-img.c

# entries
boot_sector.bin: boot_sector.ld boot_sector.o
	${LD} -o ${@:.bin=.elf} ${LDFLAGS} -T $^ -lgcc
	${OBJCOPY} -O binary --only-section=.text --only-section=.rodata --only-section=.sdata ${@:.bin=.elf} $@
	truncate -s 512 $@

loader.bin: loader.ld loader.o rfs.o stdout.o
	${LD} -o ${@:.bin=.elf} ${LDFLAGS} -T $^ -lgcc
	${OBJCOPY} -O binary --only-section=.text --only-section=.rodata --only-section=.sdata ${@:.bin=.elf} $@

clean:
	rm -f *.elf *.bin *.o bootstrap_bin.c
