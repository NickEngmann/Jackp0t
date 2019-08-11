/*
 * GENERATED FILE - DO NOT EDIT
 * (c) Code Red Technologies Ltd, 2008-2013
 * (c) NXP Semiconductors 2013-2019
 * Generated linker script file for MKL27Z64xxx4
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.23
 * MCUXpresso IDE v10.2.1 [Build 795] [2018-07-25]  on Aug 10, 2019 4:38:26 PM
 */

MEMORY
{
  /* Define each memory region */
  PROGRAM_FLASH (rx) : ORIGIN = 0x0, LENGTH = 0x1ff00 /* 130816 bytes (alias Flash) */  
  SRAM (rwx) : ORIGIN = 0x1ffff000, LENGTH = 0x4000 /* 16K bytes (alias RAM) */  
  USB_RAM (rwx) : ORIGIN = 0x400fe000, LENGTH = 0x200 /* 512 bytes (alias RAM2) */  
}

  /* Define a symbol for the top of each memory region */
  __base_PROGRAM_FLASH = 0x0  ; /* PROGRAM_FLASH */  
  __base_Flash = 0x0 ; /* Flash */  
  __top_PROGRAM_FLASH = 0x0 + 0x1ff00 ; /* 130816 bytes */  
  __top_Flash = 0x0 + 0x1ff00 ; /* 130816 bytes */  
  __base_SRAM = 0x1ffff000  ; /* SRAM */  
  __base_RAM = 0x1ffff000 ; /* RAM */  
  __top_SRAM = 0x1ffff000 + 0x4000 ; /* 16K bytes */  
  __top_RAM = 0x1ffff000 + 0x4000 ; /* 16K bytes */  
  __base_USB_RAM = 0x400fe000  ; /* USB_RAM */  
  __base_RAM2 = 0x400fe000 ; /* RAM2 */  
  __top_USB_RAM = 0x400fe000 + 0x200 ; /* 512 bytes */  
  __top_RAM2 = 0x400fe000 + 0x200 ; /* 512 bytes */  
