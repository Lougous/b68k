
ENTRY(start)

MEMORY
  {
    /* coherency to check with _K_MEM_RSVD_BLOCKS in mem.c ! */
    sys_text (!rwx) : org = 0x00000000, l = 128K
    sys_data (!rwx) : org = 0x00020000, l = 128K
  }

SECTIONS
{
  .text : { *(.text) } >sys_text
  .rodata : { *(.rodata) *(.rodata.*) } >sys_text
  .sdata :
  {
    *(.sdata)
  } >sys_data
 .bss : {
    __s_bss = . ;
    *(.bss)
    __e_bss = . ;
  } >sys_data
/*  .data : { *(.data) } >ram*/
}

