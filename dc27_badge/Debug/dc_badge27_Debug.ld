/*
 * GENERATED FILE - DO NOT EDIT
 * (c) Code Red Technologies Ltd, 2008-2013
 * (c) NXP Semiconductors 2013-2019
 * Generated linker script file for MKL27Z64xxx4
 * Created from linkscript.ldt by FMCreateLinkLibraries
 * Using Freemarker v2.3.23
 * MCUXpresso IDE v10.2.1 [Build 795] [2018-07-25]  on Aug 9, 2019 4:14:48 PM
 */

INCLUDE "dc_badge27_Debug_library.ld"
INCLUDE "dc_badge27_Debug_memory.ld"

ENTRY(ResetISR)

SECTIONS
{
    /* MAIN TEXT SECTION */
    .text : ALIGN(4)
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
        LONG(LOADADDR(.data_RAM2));
        LONG(    ADDR(.data_RAM2));
        LONG(  SIZEOF(.data_RAM2));
        __data_section_table_end = .;
        __bss_section_table = .;
        LONG(    ADDR(.bss));
        LONG(  SIZEOF(.bss));
        LONG(    ADDR(.bss_RAM2));
        LONG(  SIZEOF(.bss_RAM2));
        __bss_section_table_end = .;
        __section_table_end = . ;
        /* End of Global Section Table */

        *(.after_vectors*)

        /* Kinetis Flash Configuration data */
        . = 0x400 ;
        PROVIDE(__FLASH_CONFIG_START__ = .) ;
        KEEP(*(.FlashConfig))
        PROVIDE(__FLASH_CONFIG_END__ = .) ;
        ASSERT(!(__FLASH_CONFIG_START__ == __FLASH_CONFIG_END__), "Linker Flash Config Support Enabled, but no .FlashConfig section provided within application");
        /* End of Kinetis Flash Configuration data */
        
    } > PROGRAM_FLASH

    .text : ALIGN(4)
    {
       *(.text*)
       *(.rodata .rodata.* .constdata .constdata.*)
       . = ALIGN(4);
    } > PROGRAM_FLASH
    /*
     * for exception handling/unwind - some Newlib functions (in common
     * with C++ and STDC++) use this. 
     */
    .ARM.extab : ALIGN(4) 
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > PROGRAM_FLASH

    __exidx_start = .;

    .ARM.exidx : ALIGN(4)
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > PROGRAM_FLASH
    __exidx_end = .;

    _etext = .;
        
 
    /* USB_RAM */
    .m_usb_data (NOLOAD) :
    {
        *(m_usb_bdt)
    } > USB_RAM
    /* possible MTB section for USB_RAM */
    .mtb_buffer_RAM2 (NOLOAD) :
    {
        KEEP(*(.mtb.$RAM2*))
        KEEP(*(.mtb.$USB_RAM*))
    } > USB_RAM

    /* DATA section for USB_RAM */

    .data_RAM2 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM2 = .) ;
        *(.ramfunc.$RAM2)
        *(.ramfunc.$USB_RAM)
        *(.data.$RAM2*)
        *(.data.$USB_RAM*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM2 = .) ;
     } > USB_RAM AT>PROGRAM_FLASH
    /* MAIN DATA SECTION */
        /* Default MTB section */
        .mtb_buffer_default (NOLOAD) :
        {
           KEEP(*(.mtb*))
        } > SRAM
    .uninit_RESERVED : ALIGN(4)
    {
        KEEP(*(.bss.$RESERVED*))
        . = ALIGN(4) ;
        _end_uninit_RESERVED = .;
    } > SRAM

    /* Main DATA section (SRAM) */
    .data : ALIGN(4)
    {
       FILL(0xff)
       _data = . ;
       *(vtable)
       *(.ramfunc*)
       *(.data*)
       . = ALIGN(4) ;
       _edata = . ;
    } > SRAM AT>PROGRAM_FLASH

    /* BSS section for USB_RAM */
    .bss_RAM2 : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM2 = .) ;
       *(.bss.$RAM2*)
       *(.bss.$USB_RAM*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM2 = .) ;
    } > USB_RAM 

    /* MAIN BSS SECTION */
    .bss : ALIGN(4)
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4) ;
        _ebss = .;
        PROVIDE(end = .);
    } > SRAM

    /* NOINIT section for USB_RAM */
    .noinit_RAM2 (NOLOAD) : ALIGN(4)
    {
       *(.noinit.$RAM2*)
       *(.noinit.$USB_RAM*)
       . = ALIGN(4) ;
    } > USB_RAM 

    /* DEFAULT NOINIT SECTION */
    .noinit (NOLOAD): ALIGN(4)
    {
        _noinit = .;
        *(.noinit*) 
         . = ALIGN(4) ;
        _end_noinit = .;
    } > SRAM

    /* Reserve and place Heap within memory map */
    _HeapSize = 0x400;
    .heap :  ALIGN(4)
    {
        _pvHeapStart = .;
        . += _HeapSize;
        . = ALIGN(4);
        _pvHeapLimit = .;
    } > SRAM

     _StackSize = 0x400;
     /* Reserve space in memory for Stack */
    .heap2stackfill  :
    {
        . += _StackSize;
    } > SRAM
    /* Locate actual Stack in memory map */
    .stack ORIGIN(SRAM) + LENGTH(SRAM) - _StackSize - 0:  ALIGN(4)
    {
        _vStackBase = .;
        . = ALIGN(4);
        _vStackTop = . + _StackSize;
    } > SRAM

    /* Provide basic symbols giving location and size of main text
     * block, including initial values of RW data sections. Note that
     * these will need extending to give a complete picture with
     * complex images (e.g multiple Flash banks).
     */
    _image_start = LOADADDR(.text);
    _image_end = LOADADDR(.data) + SIZEOF(.data);
    _image_size = _image_end - _image_start;
}