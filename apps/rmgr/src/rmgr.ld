SECTIONS
{
  /* Read-only sections, merged into text segment: */
  . = 0x00180000;

  .init :
  {
    *(.init)
    startup.o(.text .rodata .data .bss COMMON)
    init.o(.text .rodata .data .bss COMMON)
    cfg-parse.o(.text .rodata .data .bss COMMON)
    cfg-mem.o(.text .rodata .data .bss COMMON)
  } =0x9090

  . = ALIGN(0x1000);

  _stext = .;

  .text      :
  {
    *(.text)
  } =0x9090

  _etext = .;
  PROVIDE (etext = .);

  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */
  . = ALIGN(0x1000) + ((ALIGN(8) + 0x1000 - ALIGN(0x1000)) & (0x1000 - 1));
  .data    :
  {
    *(.rodata)
    *(.data)
    CONSTRUCTORS
  }
  _edata  =  .;
  PROVIDE (edata = .);
  __bss_start = .;
  .bss       :
  {
   *(.bss)
   *(COMMON)
  }
  _end = . ;
  PROVIDE (end = .);
}
