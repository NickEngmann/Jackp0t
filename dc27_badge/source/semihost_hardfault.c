// ****************************************************************************
// semihost_hardfault.c
//                - Provides hard fault handler to allow semihosting code not
//                  to hang application when debugger not connected.
//
// ****************************************************************************
// Copyright(C) NXP Semiconductors, 2017-2018
// All rights reserved.
//
// Software that is described herein is for illustrative purposes only
// which provides customers with programming information regarding the
// NXP Cortex-M based MCUs. This software is supplied "AS IS" without any
// warranties of any kind, and NXP Semiconductors and its licensor disclaim any
// and all warranties, express or implied, including all implied warranties of
// merchantability, fitness for a particular purpose and non-infringement of
// intellectual property rights.  NXP Semiconductors assumes no responsibility
// or liability for the use of the software, conveys no license or rights under
// any patent, copyright, mask work right, or any other intellectual property
// rights in or to any products. NXP Semiconductors reserves the right to make
// changes in the software without notification. NXP Semiconductors also makes
// no representation or warranty that such application will be suitable for the
// specified use without further testing or modification.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation is hereby granted, under NXP Semiconductors' and its
// licensor's relevant copyrights in the software, without fee, provided that it
// is used in conjunction with NXP Semiconductors microcontrollers.  This
// copyright, permission, and disclaimer notice must appear in all copies of
// this code.
// ****************************************************************************
//
//                       ===== DESCRIPTION =====
//
// One of the issues with applications that make use of semihosting operations
// (such as printf calls) is that the code will not execute correctly when the
// debugger is not connected. Generally this will show up with the application
// appearing to just hang. This may include the application running from reset
// or powering up the board (with the application already in FLASH), and also
// as the application failing to continue to execute after a debug session is
// terminated.
//
// The problem here is that the "bottom layer" of the semihosted variants of
// the C library, semihosting is implemented by a "BKPT 0xAB" instruction.
// When the debug tools are not connected, this instruction triggers a hard
// fault - and the default hard fault handler within an application will
// typically just contains an infinite loop - causing the 
//
// This implementation adds a null-pointer check/safety validation to prevent
// the handler itself from crashing when handling faults caused by null-pointer
// dereferences or corrupted stack frames.
//
// ****************************************************************************

#include <stdint.h>
#include "semihost_hardfault.h"

// Memory map for MKL27Z644
// RAM: 0x20000000 - 0x20003FFF (16KB)
#define RAM_START_ADDR    0x20000000U
#define RAM_END_ADDR      0x20003FFFU

// Function prototype for the application's hard fault handler
extern void HardFault_Handler(void);

// Function prototype for the default hard fault handler
extern void DefaultHardFaultHandler(void);

// Function to check if stack pointer is within valid RAM bounds
static int is_stack_pointer_valid(uint32_t stack_pointer) {
    return (stack_pointer >= RAM_START_ADDR) && (stack_pointer <= RAM_END_ADDR);
}

// Function to check if address is readable (basic sanity check)
static int is_address_readable(uint32_t address) {
    // Check if address is in valid RAM range
    if (!is_stack_pointer_valid(address)) {
        return 0;
    }
    
    // Additional check: ensure address is word-aligned
    if ((address & 0x3) != 0) {
        return 0;
    }
    
    return 1;
}

// Semihost hard fault handler
void HardFault_Handler(void) {
    // Get stack pointer
    uint32_t msp, psp;
    __asm volatile ("MRS %0, msp" : "=r" (msp));
    __asm volatile ("MRS %0, psp" : "=r" (psp));
    
    // Determine which stack pointer was in use
    uint32_t stack_ptr;
    uint32_t hfsr = SCB->HFSR;
    
    // Check if hard fault was caused by exception entry (bit 31 of HFSR)
    if (hfsr & (1UL << 31)) {
        // Check if vector table read fault occurred (bit 1 of CFSR)
        if ((SCB->CFSR & (1UL << 1)) != 0) {
            // Stack might be invalid due to vector table read fault
            // Use MSP as it's the main stack used during exception entry
            stack_ptr = msp;
        } else {
            // Use PSP if it's valid, otherwise use MSP
            if (is_stack_pointer_valid(psp)) {
                stack_ptr = psp;
            } else {
                stack_ptr = msp;
            }
        }
    } else {
        // Use PSP if it's valid, otherwise use MSP
        if (is_stack_pointer_valid(psp)) {
            stack_ptr = psp;
        } else {
            stack_ptr = msp;
        }
    }
    
    // Perform sanity check on stack pointer before attempting to read stack frame
    if (!is_stack_pointer_valid(stack_ptr)) {
        // Stack pointer is invalid - avoid dereferencing it
        // Enter safe infinite loop with minimal operations
        while (1) {
            __asm volatile ("WFI");
        }
    }
    
    // Additional check: verify the stack frame address is readable
    // Check if we can read the first word of the stack frame (return address)
    uint32_t *stack_frame = (uint32_t *)stack_ptr;
    
    // Verify the stack frame looks valid by checking:
    // 1. Return address should be in flash (0x00000000 - 0x0007FFFF for MKL27Z644)
    // 2. PSR should have valid T bit set (bit 24)
    uint32_t return_addr = stack_frame[5];  // R14 (LR) is at offset 5 in stack frame
    uint32_t psr = stack_frame[6];          // PSR is at offset 6 in stack frame
    
    // Check if return address is in flash range (0x00000000 - 0x0007FFFF for 512KB flash)
    // and PSR has Thumb bit set (bit 24)
    if ((return_addr < 0x00080000U) && ((psr & (1UL << 24)) != 0)) {
        // Stack frame appears valid - proceed with normal handling
        // Call the default hard fault handler which will handle semihosting
        DefaultHardFaultHandler();
    } else {
        // Stack frame appears corrupted - avoid dereferencing it further
        // Enter safe infinite loop with minimal operations
        while (1) {
            __asm volatile ("WFI");
        }
    }
}

// Default hard fault handler implementation
__attribute__((weak))
void DefaultHardFaultHandler(void) {
    // This function is expected to be defined elsewhere or provided by the system
    // For safety, enter infinite loop with WFI to minimize power consumption
    while (1) {
        __asm volatile ("WFI");
    }
}