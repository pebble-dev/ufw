OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm",
	      "elf32-littlearm")
OUTPUT_ARCH(arm)

SECTIONS
{
  . = 0x08004000;
  
  .text : {
    *(.start);
    *(.text);
    *(SORT(.table.*));
  }
  .rodata : { *(.rodata); }
  
  . = 0x20000000;
  _data_vma = .;
  _data_lma = ADDR(.rodata) + SIZEOF(.rodata);
  .data : AT (ADDR(.rodata) + SIZEOF(.rodata)) {
    *(.data);
    . = ALIGN(4);
  }
  _data_size = SIZEOF(.data);
  
  _bss = .;
  .bss : {
    *(.bss);
    . = ALIGN(4);
  }
  _bss_size = SIZEOF(.bss);
  _bssend = .;
  _end = .;
}
