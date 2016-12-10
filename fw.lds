OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm",
	      "elf32-littlearm")
OUTPUT_ARCH(arm)

SECTIONS
{
  /* Read-only sections, merged into text segment: */
  . = 0x08004000;
  
  .text : {
    *(.start);
    *(.text);
    *(SORT(.table.*));
  }
  .data : { *(.data); *(.data2); }
  .rodata : { *(.rodata); }
  
  _bss = .;
  .bss : { *(.bss); }
  _bssend = .;
  _end = .;
}
