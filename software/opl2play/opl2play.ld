/*
** B68K Computer - Copyright (c) 2024 Lougous
** https://github.com/Lougous/b68k
*/


ENTRY(start)

MEMORY
  {
    ram (wxa)  : org = 0x00000000, l = 16K
  }

SECTIONS
{
  .ram : {
    /* put the stack at the beginning of memory section, so that any 
    overflow would cause memory protection exception */
    . = ALIGN(2);
    *(.stack)

    . = ALIGN(2);
    *(.text)
    *(.note.gnu.build-id)

    . = ALIGN(2);
    *(.rodata)
    *(.rodata.*)

    __s_bss = . ;
    *(.bss)
    __e_bss = . ;
    . = ALIGN(4);
    __s_heap = . ;
    *(.heap)
    __e_heap = . ;
  } > ram
}

