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
  .stack (NOLOAD) : {
    /* put the stack at the beginning of memory section, so that any 
    overflow would cause memory protection exception */
    . = ALIGN(4);
    *(.stack)
  } > ram

  .text : {
    . = ALIGN(4);
    *(.text .text.*)
    *(.note.gnu.build-id)
  } > ram

  .rodata : {
    . = ALIGN(4);
    *(.rodata .rodata.*)
  } > ram

  .bss : {
    . = ALIGN(4);
    __s_bss = . ;
    *(.bss .bss.*)
    . = ALIGN(4);
    __e_bss = . ;
  } > ram

  .heap (NOLOAD) : {
    . = ALIGN(4);
    __s_heap = . ;
    KEEP(*(.heap .heap.*))
    __e_heap = . ;
  } > ram
    
  /DISCARD/ : {
    /* *(.eh_frame);*/
    *(.comment);
  }
}

