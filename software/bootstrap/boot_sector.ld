
ENTRY(start)

MEMORY
  {
    /* RAM area for the boot sector */
    boot_sector (!rwx) : org = 0x00000000, l = 512
  }

PROVIDE(loader = 0x20000);
PROVIDE(stack  = 0x30000);

SECTIONS
{
  .text : { *(.text) } >boot_sector
/*
  .rodata : { *(.rodata) *(.rodata.*) } >boot_sector
  .sdata :
  {
    *(.sdata)
  } >sys_data
 .bss : {
    __s_bss = . ;
    *(.bss)
    __e_bss = . ;
  } >sys_data
*/
/*  .data : { *(.data) } >ram*/
}

