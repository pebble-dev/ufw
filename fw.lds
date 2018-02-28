OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm",
	      "elf32-littlearm")
OUTPUT_ARCH(arm)

SECTIONS
{
  . = 0x20000000;
  .text : {
    *(.start);
    *(.text);
    *(SORT(.table.*));
  }
  .rodata : { *(.rodata); }

  .data : {
    *(.data);
    . = ALIGN(4);
  }
  
  _bss = .;
  .bss : {
    *(.bss);
    . = ALIGN(4);
  }
  _bss_size = SIZEOF(.bss);
  _bssend = .;
  _end = .;
  _ram_top = 0x20000000 + 128*1024;

}
