/*
** B68K Computer - Copyright (c) 2024 Lougous
** https://github.com/Lougous/b68k
*/

ENTRY(start)

MEMORY
  {
    ram (wxa)  : org = 0x00000000, l = 1M
    null (wxa)  : org = 0x00e00000, l = 0
  }

/*
MEMORY
  {
    stack (!rw)   : org = 0x00000000, l = 512
    text (!rx)    : org = 0x00000200, l = 7K
    bss (!rw)     : org = 0x00001E00, l = 512
  }
*/

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

  .sdata :
  {
    *(.sdata)
  } > null

/*  _edata = .; PROVIDE (edata = .);*/

  /DISCARD/ : {
    /* *(.eh_frame);*/
    *(.comment);
  }
}

