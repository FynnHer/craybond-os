ENTRY(_start)
SECTIONS {
  . = 0x40000000;
  .boot . : { boot.o(.text) }
  .text : { *(.test) }
  .data : { *(.data) }
  .bss : { *(.bss COMMON) }
  . = ALIGN(16);
  . = . + 0x1000; /* 4kB of stack memory */
  stack_top = .;

  . = 0x41000000; /* or higher */
  heap_bottom = .;
}
