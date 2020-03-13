/*
 * Generated Linker script file for MK22FN512xxx12
 */

INCLUDE "../linkerscripts/firmware_library.ld"
INCLUDE "../linkerscripts/firmware_memory.ld"

ENTRY(ResetISR)

SECTIONS
{
    /* MAIN TEXT SECTION */
    .text_start : ALIGN(4)
    {
        FILL(0xff)
        __vectors_start__ = ABSOLUTE(.) ;
        KEEP(*(.isr_vector))
        /* Global Section Table */
        . = ALIGN(4) ;
        __section_table_start = .;
        __data_section_table = .;
        LONG(LOADADDR(.data));
        LONG(    ADDR(.data));
        LONG(  SIZEOF(.data));
        LONG(LOADADDR(.data_RAM4));
        LONG(    ADDR(.data_RAM4));
        LONG(  SIZEOF(.data_RAM4));
        __data_section_table_end = .;
        __bss_section_table = .;
        LONG(    ADDR(.bss));
        LONG(  SIZEOF(.bss));
        LONG(    ADDR(.bss_RAM2));
        LONG(  SIZEOF(.bss_RAM2));
        LONG(    ADDR(.bss_RAM4));
        LONG(  SIZEOF(.bss_RAM4));
        __bss_section_table_end = .;
        __section_table_end = . ;
        /* End of Global Section Table */

        *(.after_vectors*)
    } > PROGRAM_FLASH

    .codec_bin_section_1 : ALIGN(4)
    {
       . = ABSOLUTE(0x4400) ;
       *(.codec_bin_section_1)
        KEEP(*(.codec_bin_section_1))
    } > PROGRAM_FLASH

    .text_main : ALIGN(4)
    {
       *(.text*)
       *(.rodata .rodata.* .constdata .constdata.*)
       . = ALIGN(4);
       _etext = . ;
    } > PROGRAM_FLASH
    
    /* DATA section for SRAM_LOWER */
    .data_RAM4 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM2 = .) ;
        *(.ramfunc.$RAM4)
        *(.ramfunc.$SRAM_LOWER_TOP)
        *(.data.$RAM4*)
        *(.data.$SRAM_LOWER_TOP*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM2 = .) ;
     } > SRAM_LOWER_TOP AT>PROGRAM_FLASH
     
    /* DATA section for SRAM_LOWER */
    .data_AMBE : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM3 = .) ;
        *(.ramfunc.$AMBE)
        *(.ramfunc.$SRAM_LOWER_AMBE)
        *(.data.$AMBE*)
        *(.data.$SRAM_LOWER_AMBE*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM3 = .) ;
     } > SRAM_LOWER_AMBE
     
         /* Main DATA section (SRAM_UPPER) */
    .data : ALIGN(4)
    {
       FILL(0xff)
       _data = . ;
       *(vtable)
       *(.ramfunc*)
       *(.data*)
       . = ALIGN(4) ;
       _edata = . ;
    } > SRAM_LOWER_BOTTOM AT>PROGRAM_FLASH
     
    /* MAIN DATA SECTION */
    .uninit_RESERVED : ALIGN(4)
    {
        KEEP(*(.bss.$RESERVED*))
        . = ALIGN(4) ;
        _end_uninit_RESERVED = .;
    } > SRAM_UPPER


    .codec_bin_section_2 : ALIGN(4)
    {
        . = ABSOLUTE(0x50000) ;
       *(.codec_bin_section_2)
        KEEP(*(.codec_bin_section_2))
    } > PROGRAM_FLASH

    /* BSS section for SRAM_LOWER */
    .bss_RAM2 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM2 = .) ;
       *(.bss.$RAM2*)
       *(.bss.$SRAM_LOWER_BOTTOM*)
       *(COMMON)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM2 = .) ;
    } > SRAM_LOWER_BOTTOM 

    /* BSS section for SRAM_LOWER */
    .bss_RAM4 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM4 = .) ;
       *(.bss.$RAM4*)
       *(.bss.$SRAM_LOWER_TOP*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM4 = .) ;
    } > SRAM_LOWER_TOP 

    /* MAIN BSS SECTION */
    .bss : ALIGN(4)
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4) ;
        _ebss = .;
        PROVIDE(end = .);
    } > SRAM_UPPER

    /* NOINIT section for SRAM_LOWER_BOTTOM */
    .noinit_RAM2 (NOLOAD) : ALIGN(4)
    {
       *(.noinit.$RAM2*)
       *(.noinit.$SRAM_LOWER_BOTTOM*)
       . = ALIGN(4) ;
    } > SRAM_LOWER_BOTTOM 
    
    /* NOINIT section for SRAM_LOWER_AMBE */
    .noinit_AMBE (NOLOAD) : ALIGN(4)
    {
       *(.noinit.$AMBE*)
       *(.noinit.$SRAM_LOWER_AMBE*)
       . = ALIGN(4) ;
    } > SRAM_LOWER_AMBE 

    /* NOINIT section for SRAM_LOWER_TOP */
    .noinit_RAM4 (NOLOAD) : ALIGN(4)
    {
       *(.noinit.$RAM4*)
       *(.noinit.$SRAM_LOWER_TOP*)
       . = ALIGN(4) ;
    } > SRAM_LOWER_TOP 

    /* DEFAULT NOINIT SECTION */
    .noinit (NOLOAD): ALIGN(4)
    {
        _noinit = .;
        *(.noinit*) 
         . = ALIGN(4) ;
        _end_noinit = .;
    } > SRAM_UPPER

    /* Reserve and place Heap within memory map */
    _HeapSize = 0x1000;
    .heap :  ALIGN(4)
    {
        _pvHeapStart = .;
        . += _HeapSize;
        . = ALIGN(4);
        _pvHeapLimit = .;
    } > SRAM_LOWER_TOP

     _StackSize = 0x1000;
     /* Reserve space in memory for Stack */
    .heap2stackfill  :
    {
        . += _StackSize;
    } > SRAM_LOWER_TOP
    
    /* Locate actual Stack in memory map */
    .stack ORIGIN(SRAM_LOWER_TOP) + LENGTH(SRAM_LOWER_TOP) - _StackSize - 0:  ALIGN(4)
    {
        _vStackBase = .;
        . = ALIGN(4);
        _vStackTop = . + _StackSize;
    } > SRAM_LOWER_TOP

    /* Provide basic symbols giving location and size of main text
     * block, including initial values of RW data sections. Note that
     * these will need extending to give a complete picture with
     * complex images (e.g multiple Flash banks).
     */
    _image_start = LOADADDR(.text_start);
    _image_end = LOADADDR(.codec_bin_section_2) + SIZEOF(.codec_bin_section_2);
    _image_size = _image_end - _image_start;
}
