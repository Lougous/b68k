
ENTRY(start)

MEMORY
  {
    /* reserve RAM area for the boot */
/*  rsvd (!rwx) : org = 0x00000000, l = 64K */
    sys_text (!rwx) : org = 0x00000000, l = 64K
    sys_data (!rwx) : org = 0x00010000, l = 128K
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

