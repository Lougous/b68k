/*
** B68K Computer - Copyright (c) 2024 Lougous
** https://github.com/Lougous/b68k
**
** System/binaries - linker script
*/

ENTRY(start)

MEMORY
{
ram (wxa)  : org = 0x00000000, l = 64K
}

SECTIONS
{
  .ram : {
    /* put the stack at the beginning of memory section, so that any 
    overflow would cause memory protection exception */
    . = ALIGN(4);
    *(.stack)

    . = ALIGN(4);
    *(.text)
    *(.note.gnu.build-id)

    . = ALIGN(4);
    *(.rodata)
    *(.rodata.*)

    . = ALIGN(4);
    __s_bss = . ;
    *(.bss)
    . = ALIGN(4);
    __e_bss = . ;
    
    . = ALIGN(4);
    __s_heap = . ;
    *(.heap)
    __e_heap = . ;
  } > ram
}

