
ENTRY(loader)

MEMORY
  {
    sys_text (!rwx) : org = 0x00020000, l = 4K
    sys_data (!rwx) : org = 0x00024000, l = 4K
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

