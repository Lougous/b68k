
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
  .ram : {
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
    *(.data)
    __s_heap = . ;
    *(.heap)
    __e_heap = . ;
  } > ram

/*
  .text : {
    . = ALIGN(2);
    *(.text)
    *(.note.gnu.build-id)
  } > ram

  .rodata : {
    . = ALIGN(2);
    *(.rodata)
    *(.rodata.*)
  } > ram
  */
  
/*  mmap_sdata = .*/
  .sdata :
  {
    *(.sdata)
  } > null

/*  _edata = .; PROVIDE (edata = .);*/

}

