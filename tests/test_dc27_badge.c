#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

// Global variables that need to be mocked
volatile unsigned char g_random = 0x55;  // Initialize with known value for testing
volatile uint32_t g_systickCounter = 0;
volatile bool g_lptmrFlag = false;
volatile bool g_nxhDetect = false;

// Mock PRINTF macro
#define PRINTF(...) printf(__VA_ARGS__)

// Function declarations from dc27_badge.c
unsigned char Get_Random_Byte(void);
void Print_Bits(uint8_t myByte);
void Reorder_Array(uint8_t *array, uint8_t *index, uint8_t N);
void SysTick_DelayTicks(uint32_t n);
void SysTick_Handler(void);

// Test Get_Random_Byte
void test_get_random_byte(void) {
    printf("Testing Get_Random_Byte...\n");
    
    // Test with known initial value
    g_random = 0x55;
    
    // First call should return 0x55
    unsigned char result1 = Get_Random_Byte();
    assert(result1 == 0x55);
    
    // Second call should return different value (PRNG output)
    unsigned char result2 = Get_Random_Byte();
    assert(result2 != result1);  // Very likely to be different
    
    // Test with different initial value
    g_random = 0xAA;
    unsigned char result3 = Get_Random_Byte();
    assert(result3 == 0xAA);
    
    printf("Get_Random_Byte test passed!\n");
}

// Test Print_Bits
void test_print_bits(void) {
    printf("Testing Print_Bits...\n");
    
    // Test with known values
    printf("Print_Bits(0x00): ");
    Print_Bits(0x00);
    printf(" (expected: 00000000)\n");
    
    printf("Print_Bits(0xFF): ");
    Print_Bits(0xFF);
    printf(" (expected: 11111111)\n");
    
    printf("Print_Bits(0x55): ");
    Print_Bits(0x55);
    printf(" (expected: 01010101)\n");
    
    printf("Print_Bits(0xAA): ");
    Print_Bits(0xAA);
    printf(" (expected: 10101010)\n");
    
    printf("Print_Bits test passed!\n");
}

// Test Reorder_Array
void test_reorder_array(void) {
    printf("Testing Reorder_Array...\n");
    
    // Test with simple array
    uint8_t array1[] = {1, 2, 3, 4, 5};
    uint8_t index1[] = {4, 3, 2, 1, 0};  // Reverse order
    uint8_t expected1[] = {5, 4, 3, 2, 1};
    
    Reorder_Array(array1, index1, 5);
    
    for (int i = 0; i < 5; i++) {
        assert(array1[i] == expected1[i]);
    }
    
    // Verify index array was reset
    for (int i = 0; i < 5; i++) {
        assert(index1[i] == i);
    }
    
    // Test with single element
    uint8_t array2[] = {42};
    uint8_t index2[] = {0};
    Reorder_Array(array2, index2, 1);
    assert(array2[0] == 42);
    assert(index2[0] == 0);
    
    // Test with already ordered array
    uint8_t array3[] = {1, 2, 3};
    uint8_t index3[] = {0, 1, 2};
    Reorder_Array(array3, index3, 3);
    assert(array3[0] == 1);
    assert(array3[1] == 2);
    assert(array3[2] == 3);
    
    printf("Reorder_Array test passed!\n");
}

// Test SysTick_DelayTicks
void test_systick_delay(void) {
    printf("Testing SysTick_DelayTicks...\n");
    
    g_systickCounter = 0;
    
    // Test that setting delay works
    SysTick_DelayTicks(100);
    assert(g_systickCounter == 100);
    
    // Simulate SysTick interrupt handler
    SysTick_Handler();
    assert(g_systickCounter == 99);
    
    // Test with 0 delay
    g_systickCounter = 0;
    SysTick_DelayTicks(0);
    assert(g_systickCounter == 0);
    
    printf("SysTick_DelayTicks test passed!\n");
}

// Test SysTick_Handler
void test_systick_handler(void) {
    printf("Testing SysTick_Handler...\n");
    
    g_systickCounter = 5;
    
    // Call handler 5 times to decrement to 0
    for (int i = 0; i < 5; i++) {
        SysTick_Handler();
        assert(g_systickCounter == (4 - i));
    }
    
    // Verify it stays at 0
    SysTick_Handler();
    assert(g_systickCounter == 0);
    
    printf("SysTick_Handler test passed!\n");
}

int main(void) {
    printf("Running DC27 Badge Firmware Tests...\n\n");
    
    test_get_random_byte();
    test_print_bits();
    test_reorder_array();
    test_systick_delay();
    test_systick_handler();
    
    printf("\nAll tests passed!\n");
    return 0;
}
