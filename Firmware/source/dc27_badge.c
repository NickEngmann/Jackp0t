/*

  DEFCON 27 Official Badge (2019)

  Author: Joe Grand, Grand Idea Studio [@joegrand] aka Kingpin

  Program Description:

  This program contains the firmware for the DEFCON 27 official badge.

  Complete design documentation can be found at:
  http://www.grandideastudio.com/portfolio/defcon-27-badge

*/

// Portions of this code are subject to the following license:
/*
 * The Clear BSD License
 * Copyright 2016-2018 NXP Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/**
 * @file    dc27_badge.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKL27Z644.h"
#include "fsl_debug_console.h"
#include "fsl_smc.h"
#include "fsl_flash.h"

#include "LPBroadcast_NXH_DC27.eep.h"  // Pre-compiled firmware blob for NXH2261 (loaded during power-up)


/***************************************************************************
 **************************** Definitions **********************************
 ***************************************************************************/

#define __BADGE_TYPE		HUMAN
//#define __BADGE_TYPE		GOON
//#define __BADGE_TYPE		SPEAKER
//#define __BADGE_TYPE		VENDOR
//#define __BADGE_TYPE		PRESS
//#define __BADGE_TYPE		VILLAGE
//#define __BADGE_TYPE		CONTEST
//#define __BADGE_TYPE		ARTIST
//#define __BADGE_TYPE		CFP
//#define __BADGE_TYPE		UBER

#undef __BADGE_MAGIC
//#define __BADGE_MAGIC

#define LOW		0U
#define HIGH	1U

// LED animation
#define LED_HEARTBEAT_FADE_DELAY		6		// Time (ms) per brightness setting ramp up/down
#define LED_HEARTBEAT_WAIT_DELAY		1000	// Time (ms) to sleep at LED maximum brightness

#define LED_SPARKLE_FADE_DELAY			2		// Time (ms) per brightness setting ramp up/down
#define LED_SPARKLE_ON_DELAY			350		// Time (ms) to remain on at LED maximum brightness
#define LED_SPARKLE_WAIT_DELAY			1500 	// Time (ms) to sleep between LED updates

// Bit masks for badge quest flags
#define FLAG_0_MASK						0x01	// Any Valid Communication
#define FLAG_1_MASK						0x02	// Talk/Speaker
#define FLAG_2_MASK						0x04	// Village
#define FLAG_3_MASK						0x08	// Contest & Events
#define FLAG_4_MASK						0x10	// Arts & Entertainment
#define FLAG_5_MASK						0x20	// Parties
#define FLAG_6_MASK						0x40	// Group Chat
#define FLAG_ALL_MASK					0x7F
#define GROUP_ALL_MASK					0x3F

#define CONSOLE_RCVBUF_SIZE				20  	// Number of bytes in debug console (interactive mode) receive buffer
#define ART_DEFAULT						"DC27"	// Default string for ASCII art generator

#define NVM_DATA_SIZE 					4U		// Number of bytes to store in KL27 Flash
#define SECTOR_INDEX_FROM_END 			1U		// Location of KL27 Flash sector to use for game data storage

// CLKOUT
#define SIM_CLKOUT_SEL_OSCERCLK_CLK     6U 		// CLKOUT pin clock select: OSCERCLK (from clock_config.c)

// LPUART0 (to/from NXH2261)
#define LPUART0_RING_BUFFER_SIZE 		2048U	// Number of bytes in ring buffer for receiving data from NXH

// I2C
#define I2C_NXH2261_ADDR   		0x10
#define I2C_LP5569_ADDR			0x32

// NXH2261
#define NXH2261_DATA_PACKET_SIZE		18U		 // header + 16 user bytes + footer
#define NXH2261_MAX_CHUNK_SIZE			128U
#define NXH2261_CMD_GET_VERSION			0x0F80	 // Get device version information
#define NXH2261_CMD_PREVENT_BOOT		0x0F16	 // Aborts automatic boot procedure, puts device into bootloader
#define NXH2261_CMD_EEPROM_ENABLE		0x0F18	 // Enables the EEPROM (required before any EEPROM manipulation)
#define NXH2261_CMD_EEPROM_DISABLE		0x0F19	 // Disables the EEPROM
#define NXH2261_CMD_EEPROM_UNLOCK		0x0F0C   // Unlocks EEPROM memory location for a single write
#define NXH2261_CMD_EEPROM_READ			0x0F0B	 // Read data from EEPROM
#define NXH2261_CMD_EEPROM_WRITE		0x0F0A	 // Write data to EEPROM
#define NXH2261_CMD_START_APP			0x0F00	 // Executes the application currently in RAM
#define NXH2261_CMD_EEPROM_BOOT			0x0F0F   // Copies application from EEPROM into RAM and executes

// LP5569
#define LP5569_LED_NUM			6U  	// Number of LEDs per badge

#define LED_CONTROL_RED  		0x08	// No master fading, exponential adjustment, LED powered by charge pump
#define LED_CURRENT_RED			0x0A 	// 1.0mA
#define LED_PWM_RED				0xC0	// 75% duty cycle

#define LED_CONTROL_ORANGE  	0x08	// No master fading, exponential adjustment, LED powered by charge pump
#define LED_CURRENT_ORANGE		0x07 	// 0.7mA
#define LED_PWM_ORANGE			0xC0	// 75% duty cycle

#define LED_CONTROL_GREEN  		0x08	// No master fading, exponential adjustment, LED powered by charge pump
#define LED_CURRENT_GREEN		0x02 	// 0.2mA
#define LED_PWM_GREEN			0xC0	// 75% duty cycle

#define LED_CONTROL_BLUE  		0x08	// No master fading, exponential adjustment, LED powered by charge pump
#define LED_CURRENT_BLUE		0x07 	// 0.7mA
#define LED_PWM_BLUE			0xC0	// 75% duty cycle

#define LED_CONTROL_PURPLE  	0x08	// No master fading, exponential adjustment, LED powered by charge pump
#define LED_CURRENT_PURPLE		0x07 	// 0.7mA
#define LED_PWM_PURPLE			0xC0	// 75% duty cycle

#define LED_CONTROL_WHITE  		0x08	// No master fading, exponential adjustment, LED powered by charge pump
#define LED_CURRENT_WHITE		0x06 	// 0.6mA
#define LED_PWM_WHITE			0xC0	// 75% duty cycle

// From Section 8.6: Register Maps
#define LP5569_REG_CONFIG                   0x00 	// Configuration Register
#define LP5569_REG_LED_ENGINE_CONTROL1		0x01 	// Engine Execution Control Register
#define LP5569_REG_LED_ENGINE_CONTROL2      0x02 	// Engine Operation Mode Register
#define LP5569_REG_LED0_CONTROL             0x07 	// LED0 Control Register
#define LP5569_REG_LED1_CONTROL             0x08 	// LED1 Control Register
#define LP5569_REG_LED2_CONTROL             0x09 	// LED2 Control Register
#define LP5569_REG_LED3_CONTROL             0x0A 	// LED3 Control Register
#define LP5569_REG_LED4_CONTROL             0x0B 	// LED4 Control Register
#define LP5569_REG_LED5_CONTROL             0x0C 	// LED5 Control Register
#define LP5569_REG_LED6_CONTROL             0x0D 	// LED6 Control Register
#define LP5569_REG_LED7_CONTROL             0x0E 	// LED7 Control Register
#define LP5569_REG_LED8_CONTROL             0x0F 	// LED8 Control Register
#define LP5569_REG_LED0_PWM                 0x16 	// LED0 PWM Duty Cycle
#define LP5569_REG_LED1_PWM                 0x17 	// LED1 PWM Duty Cycle
#define LP5569_REG_LED2_PWM                 0x18 	// LED2 PWM Duty Cycle
#define LP5569_REG_LED3_PWM                 0x19 	// LED3 PWM Duty Cycle
#define LP5569_REG_LED4_PWM                 0x1A 	// LED4 PWM Duty Cycle
#define LP5569_REG_LED5_PWM                 0x1B 	// LED5 PWM Duty Cycle
#define LP5569_REG_LED6_PWM                 0x1C 	// LED6 PWM Duty Cycle
#define LP5569_REG_LED7_PWM                 0x1D 	// LED7 PWM Duty Cycle
#define LP5569_REG_LED8_PWM                 0x1E 	// LED8 PWM Duty Cycle
#define LP5569_REG_LED0_CURRENT             0x22 	// LED0 Current Control
#define LP5569_REG_LED1_CURRENT             0x23 	// LED1 Current Control
#define LP5569_REG_LED2_CURRENT             0x24 	// LED2 Current Control
#define LP5569_REG_LED3_CURRENT             0x25 	// LED3 Current Control
#define LP5569_REG_LED4_CURRENT             0x26 	// LED4 Current Control
#define LP5569_REG_LED5_CURRENT             0x27 	// LED5 Current Control
#define LP5569_REG_LED6_CURRENT             0x28 	// LED6 Current Control
#define LP5569_REG_LED7_CURRENT             0x29 	// LED7 Current Control
#define LP5569_REG_LED8_CURRENT             0x2A 	// LED8 Current Control
#define LP5569_REG_MISC                     0x2F 	// I2C Charge Pump and Clock Configuration
#define LP5569_REG_ENGINE1_PC               0x30 	// Engine 1 Program Counter
#define LP5569_REG_ENGINE2_PC               0x31 	// Engine 2 Program Counter
#define LP5569_REG_ENGINE3_PC               0x32 	// Engine 3 Program Counter
#define LP5569_REG_MISC2                    0x33 	// Charge Pump and LED Configuration
#define LP5569_REG_ENGINE_STATUS            0x3C 	// Engine 1/2/3 Status
#define LP5569_REG_IO_CONTROL               0x3D 	// TRIG/INT/CLK Configuration
#define LP5569_REG_VARIABLE_D               0x3E 	// Global Variable D
#define LP5569_REG_RESET                    0x3F 	// Software Reset
#define LP5569_REG_ENGINE1_VARIABLE_A       0x42 	// Engine 1 Local Variable A
#define LP5569_REG_ENGINE2_VARIABLE_A       0x43 	// Engine 2 Local Variable A
#define LP5569_REG_ENGINE3_VARIABLE_A       0x44 	// Engine 3 Local Variable A
#define LP5569_REG_MASTER_FADER1            0x46 	// Engine 1 Master Fader
#define LP5569_REG_MASTER_FADER2            0x47 	// Engine 2 Master Fader
#define LP5569_REG_MASTER_FADER3            0x48 	// Engine 3 Master Fader
#define LP5569_REG_MASTER_FADER_PWM         0x4A 	// PWM Input Duty Cycle
#define LP5569_REG_ENGINE1_PROG_START       0x4B 	// Engine 1 Program Starting Address
#define LP5569_REG_ENGINE2_PROG_START       0x4C 	// Engine 2 Program Starting Address
#define LP5569_REG_ENGINE3_PROG_START       0x4D 	// Engine 3 Program Starting Address
#define LP5569_REG_PROG_MEM_PAGE_SELECT     0x4F 	// Program Memory Page Select
#define LP5569_REG_PROGRAM_MEM_00           0x50 	// MSB 0
#define LP5569_REG_PROGRAM_MEM_01           0x51 	// LSB 0
#define LP5569_REG_PROGRAM_MEM_02           0x52 	// MSB 1
#define LP5569_REG_PROGRAM_MEM_03           0x53 	// LSB 1
#define LP5569_REG_PROGRAM_MEM_04           0x54 	// MSB 2
#define LP5569_REG_PROGRAM_MEM_05           0x55 	// LSB 2
#define LP5569_REG_PROGRAM_MEM_06           0x56 	// MSB 3
#define LP5569_REG_PROGRAM_MEM_07           0x57 	// LSB 3
#define LP5569_REG_PROGRAM_MEM_08           0x58 	// MSB 4
#define LP5569_REG_PROGRAM_MEM_09           0x59 	// LSB 4
#define LP5569_REG_PROGRAM_MEM_10           0x5A 	// MSB 5
#define LP5569_REG_PROGRAM_MEM_11           0x5B 	// LSB 5
#define LP5569_REG_PROGRAM_MEM_12           0x5C 	// MSB 6
#define LP5569_REG_PROGRAM_MEM_13           0x5D 	// LSB 6
#define LP5569_REG_PROGRAM_MEM_14           0x5E 	// MSB 7
#define LP5569_REG_PROGRAM_MEM_15           0x5F 	// LSB 7
#define LP5569_REG_PROGRAM_MEM_16           0x60 	// MSB 8
#define LP5569_REG_PROGRAM_MEM_17           0x61 	// LSB 8
#define LP5569_REG_PROGRAM_MEM_18           0x62 	// MSB 9
#define LP5569_REG_PROGRAM_MEM_19           0x63 	// LSB 9
#define LP5569_REG_PROGRAM_MEM_20           0x64 	// MSB 10
#define LP5569_REG_PROGRAM_MEM_21           0x65 	// LSB 10
#define LP5569_REG_PROGRAM_MEM_22           0x66 	// MSB 11
#define LP5569_REG_PROGRAM_MEM_23           0x67 	// LSB 11
#define LP5569_REG_PROGRAM_MEM_24           0x68 	// MSB 12
#define LP5569_REG_PROGRAM_MEM_25           0x69 	// LSB 12
#define LP5569_REG_PROGRAM_MEM_26           0x6A 	// MSB 13
#define LP5569_REG_PROGRAM_MEM_27           0x6B 	// LSB 13
#define LP5569_REG_PROGRAM_MEM_28           0x6C 	// MSB 14
#define LP5569_REG_PROGRAM_MEM_29           0x6D 	// LSB 14
#define LP5569_REG_PROGRAM_MEM_30           0x6E 	// MSB 15
#define LP5569_REG_PROGRAM_MEM_31           0x6F 	// LSB 15
#define LP5569_REG_ENGINE1_MAPPING1         0x70 	// Engine 1 LED [8] and Master Fader Mapping
#define LP5569_REG_ENGINE1_MAPPING2         0x71 	// Engine 1 LED [7:0] Mapping
#define LP5569_REG_ENGINE2_MAPPING1         0x72 	// Engine 2 LED [8] and Master Fader Mapping
#define LP5569_REG_ENGINE2_MAPPING2         0x73 	// Engine 2 LED [7:0] Mapping
#define LP5569_REG_ENGINE3_MAPPING1         0x74 	// Engine 3 LED [8] and Master Fader Mapping
#define LP5569_REG_ENGINE3_MAPPING2         0x75 	// Engine 3 LED [7:0] Mapping
#define LP5569_REG_PWM_CONFIG               0x80 	// PWM Input Configuration
#define LP5569_REG_LED_FAULT1               0x81 	// LED [8] Fault Status
#define LP5569_REG_LED_FAULT2               0x82 	// LED [7:0] Fault Status
#define LP5569_REG_GENERAL_FAULT            0x83 	// CP Cap UVLO and TSD Fault Status

// Piezo
// Table of notes/pitch (in Hz)
#define NOTE_REST	0
#define NOTE_B0  	31
#define NOTE_C1  	33
#define NOTE_CS1 	35
#define NOTE_D1  	37
#define NOTE_DS1 	39
#define NOTE_E1  	41
#define NOTE_F1  	44
#define NOTE_FS1 	46
#define NOTE_G1  	49
#define NOTE_GS1 	52
#define NOTE_A1  	55
#define NOTE_AS1 	58
#define NOTE_B1  	62
#define NOTE_C2  	65
#define NOTE_CS2 	69
#define NOTE_D2  	73
#define NOTE_DS2 	78
#define NOTE_E2  	82
#define NOTE_F2  	87
#define NOTE_FS2 	93
#define NOTE_G2  	98
#define NOTE_GS2 	104
#define NOTE_A2  	110
#define NOTE_AS2 	117
#define NOTE_B2  	123
#define NOTE_C3  	131
#define NOTE_CS3 	139
#define NOTE_D3  	147
#define NOTE_DS3 	156
#define NOTE_E3  	165
#define NOTE_F3  	175
#define NOTE_FS3 	185
#define NOTE_G3  	196
#define NOTE_GS3 	208
#define NOTE_A3  	220
#define NOTE_AS3 	233
#define NOTE_B3  	247
#define NOTE_C4  	262
#define NOTE_CS4 	277
#define NOTE_D4  	294
#define NOTE_DS4 	311
#define NOTE_E4  	330
#define NOTE_F4  	349
#define NOTE_FS4 	370
#define NOTE_G4  	392
#define NOTE_GS4 	415
#define NOTE_A4  	440
#define NOTE_AS4 	466
#define NOTE_B4  	494
#define NOTE_C5  	523
#define NOTE_CS5 	554
#define NOTE_D5  	587
#define NOTE_DS5 	622
#define NOTE_E5  	659
#define NOTE_F5  	698
#define NOTE_FS5 	740
#define NOTE_G5  	784
#define NOTE_GS5 	831
#define NOTE_A5 	880
#define NOTE_AS5 	932
#define NOTE_B5  	988
#define NOTE_C6  	1047
#define NOTE_CS6 	1109
#define NOTE_D6  	1175
#define NOTE_DS6 	1245
#define NOTE_E6  	1319
#define NOTE_F6  	1397
#define NOTE_FS6 	1480
#define NOTE_G6  	1568
#define NOTE_GS6 	1661
#define NOTE_A6  	1760
#define NOTE_AS6 	1865
#define NOTE_B6  	1976
#define NOTE_C7  	2093
#define NOTE_CS7 	2217
#define NOTE_D7  	2349
#define NOTE_DS7 	2489
#define NOTE_E7  	2637
#define NOTE_F7  	2794
#define NOTE_FS7 	2960
#define NOTE_G7  	3136
#define NOTE_GS7 	3322
#define NOTE_A7  	3520
#define NOTE_AS7 	3729
#define NOTE_B7  	3951
#define NOTE_C8  	4186
#define NOTE_CS8 	4435
#define NOTE_D8  	4699
#define NOTE_DS8 	4978


/**************************************************************************
************************** Macros *****************************************
***************************************************************************/

#define HIGH_BYTE(x)  ((x) >> 8)
#define LOW_BYTE(x)   ((x) & 0xFF)

#define dc27_invalid_cmd() 	PRINTF("?")


/**************************************************************************
************************** Structs ****************************************
***************************************************************************/

typedef enum	// badge types
{
	HUMAN,
	GOON,
	SPEAKER,
	VENDOR,
	PRESS,
	VILLAGE,
	CONTEST,
	ARTIST,
	CFP,
	UBER
} badge_type_t;

typedef enum	// badge states
{
	ATTRACT,
	D,
	E,
	F,
	C,
	O,
	N,
	COMPLETE
} badge_state_t;

struct packet_of_infamy  // data packet for NFMI transfer
{
	uint32_t uid;		// unique ID
	uint8_t type;		// badge type
	uint8_t	magic;		// magic token (1 = enabled)
	uint8_t flags;		// game flags (packed, MSB unused)
	uint8_t unused;		// unused
};

struct note		// sound generation
{
   long freq;		// frequency (Hz)
   long duration;	// duration (ms)
   char duty;		// duty cycle (%)
};


/****************************************************************************
 ************************** Global variables ********************************
 ***************************************************************************/

// Badge
volatile static badge_state_t badge_state, attract_state;
volatile static badge_type_t badge_type = __BADGE_TYPE;
volatile static uint8_t game_flags, group_flags;  // tasks to complete, 1 = done, 0 = not done
volatile unsigned char g_random; // PRNG
volatile bool g_oldRx, g_newRx;  // used to detect rising edge transition of KL RX (for interactive mode)

// Flash driver/EEPROM
static flash_config_t s_flashDriver;
static ftfx_cache_config_t s_cacheDriver;
static uint32_t pflashBlockBase = 0;
static uint32_t pflashTotalSize = 0;
static uint32_t pflashSectorSize = 0;

// Timer
volatile uint32_t g_systickCounter;
volatile bool g_lptmrFlag;

// UART2 (to/from host)
extern const uart_config_t UART2_config;	// peripherals.c

// LPUART0 (to/from NXH2261)
/*
  Ring buffer for data input and output
  Input data is stored to the ring buffer in IRQ handler
  Ring buffer full: (((rxIndex + 1) % RING_BUFFER_SIZE) == txIndex)
  Ring buffer empty: (rxIndex == txIndex)
*/
volatile static uint8_t nxhRingBuffer[LPUART0_RING_BUFFER_SIZE];
volatile static uint16_t nxhTxIndex; 	// Index of the data to send out
volatile static uint16_t nxhRxIndex; 	// Index of the memory to save new received data

// LP5569 LED driver default settings
unsigned char LP5569_Control;	// Control Register
unsigned char LP5569_Current;	// Current Control
unsigned char LP5569_PWM;		// PWM Duty Cycle

// NHX2261
volatile bool g_nxhDetect = false;
static struct packet_of_infamy nxhTxPacket; 	// Data packet to transmit
static struct packet_of_infamy nxhRxPacket; 	// Received data packet

// Piezo/PWM
extern const tpm_chnl_pwm_signal_param_t TPM0_pwmSignalParams[];  	// peripherals.c


/****************************************************************************
 *************************** Constants **************************************
 ***************************************************************************/

const char command_prompt[] = "\n\r> ";

const char menu_banner[] = "\n\r\
T: Display transmit packet\n\r\
R: Receive packet(s)\n\r\
C: Clear game flags\n\r\
H: Display available commands\n\r\
^: System reset\n\r\
Ctrl-X: Exit interactive mode\n\r\
";

const char menu_banner_complete[] = "\n\r\
A <string>: ASCII art generator\n\r\
S <freq> <ms>: Tone generator\n\r\
U <hex bytes>: Update transmit packet\n\r\
";

const char msg_welcome[]          = "\n\r\n\rWelcome to the DEFCON 27 Official Badge\n\r\n\r";
const char msg_init_start[]       = "[*] Starting Initialization\n\r";
const char msg_init_complete[]    = "[*] Initialization Complete\n\r";
const char msg_interactive_mode[] = "[*] Entering Interactive Mode [Press 'H' for Commands]\n\r";
const char msg_interactive_exit[] = "\n\r[*] Exiting Interactive Mode\n\r";
const char msg_nfmi_packet_err[]  = "[*] NFMI Packet Update Error!\n\r";

const char msg_version[] = "\
Designed by Joe Grand [@joegrand] aka Kingpin\n\r\
grandideastudio.com/portfolio/defcon-27-badge/\n\r\n\r\
";

// NES Super Mario Bros. 1-Up
// Sheet music from http://www.mariopiano.com/mario-sheet-music-1-up-mushroom-sound.html
const struct note tune_1up[] = {
	{NOTE_E6, 125, 50}, {NOTE_G6, 125, 50}, {NOTE_E7, 125, 50}, {NOTE_C7, 125, 50},
	{NOTE_D7, 125, 50}, {NOTE_G7, 125, 50}
};

// He Who Shall Not Be Named
// Ported from https://create.arduino.cc/projecthub/slagestee/rickroll-box-3c2245
/*const struct note tune_rickroll_intro[] = {
	{NOTE_CS5, 600, 50}, {NOTE_DS5, 1000, 50}, {NOTE_DS5, 600, 50}, {NOTE_F5, 600, 50},
	{NOTE_GS5, 100, 50}, {NOTE_FS5, 100, 50}, {NOTE_F5, 100, 50}, {NOTE_DS5, 100, 50},
	{NOTE_CS5, 600, 50}, {NOTE_DS5, 1000, 50}, {NOTE_REST, 400, 50}, {NOTE_GS4, 200, 50},
	{NOTE_GS4, 1000, 50}
};*/

/*const struct note tune_rickroll_verse[] = {
	{NOTE_REST, 400, 50}, {NOTE_CS4, 200, 50}, {NOTE_CS4, 200, 50}, {NOTE_CS4, 200, 50},
	{NOTE_CS4, 200, 50}, {NOTE_DS4, 400, 50}, {NOTE_REST, 200, 50}, {NOTE_C4, 200, 50},
	{NOTE_AS3, 200, 50}, {NOTE_GS3, 1000, 50}, {NOTE_REST, 200, 50}, {NOTE_AS3, 200, 50},
	{NOTE_AS3, 200, 50}, {NOTE_C4, 200, 50}, {NOTE_CS4, 600, 50}, {NOTE_GS3, 200, 50},
	{NOTE_GS4, 400, 50}, {NOTE_GS4, 200, 50}, {NOTE_DS4, 1000, 50}, {NOTE_REST, 200, 50},
	{NOTE_AS3, 200, 50}, {NOTE_AS3, 200, 50}, {NOTE_C4, 200, 50}, {NOTE_CS4, 200, 50},
	{NOTE_AS3, 200, 50}, {NOTE_CS4, 200, 50}, {NOTE_DS4, 400, 50}, {NOTE_REST, 200, 50},
	{NOTE_C4, 200, 50}, {NOTE_AS3, 200, 50}, {NOTE_AS3, 200, 50}, {NOTE_GS3, 600, 50},
	{NOTE_REST, 200, 50}, {NOTE_AS3, 200, 50}, {NOTE_AS3, 200, 50}, {NOTE_C4, 200, 50},
	{NOTE_CS4, 400, 50}, {NOTE_GS3, 200, 50}, {NOTE_GS3, 200, 50}, {NOTE_DS4, 200, 50},
	{NOTE_DS4, 200, 50}, {NOTE_DS4, 200, 50}, {NOTE_F4, 200, 50}, {NOTE_DS4, 800, 50},
	{NOTE_CS4, 1000, 50}, {NOTE_DS4, 200, 50}, {NOTE_F4, 200, 50}, {NOTE_CS4, 200, 50},
	{NOTE_DS4, 200, 50}, {NOTE_DS4, 200, 50}, {NOTE_DS4, 200, 50}, {NOTE_F4, 200, 50},
	{NOTE_DS4, 400, 50}, {NOTE_GS3, 400, 50}, {NOTE_REST, 400, 50}, {NOTE_AS3, 200, 50},
	{NOTE_C4, 200, 50}, {NOTE_CS4, 200, 50}, {NOTE_GS3, 600, 50}, {NOTE_REST, 200, 50},
	{NOTE_DS4, 200, 50}, {NOTE_F4, 200, 50}, {NOTE_DS4, 600, 50}
};

const char* tune_rickroll_verse_lyrics[] = {
	"\n\r", "We're ", "no ", "stran", "gers ", "", "to ", "", "love ", "", "\n\r",
	"You ", "know ", "the ", "rules ", "and ", "so ", "do ", "I", "", "\n\r",
	"A ", "full ", "com", "mit", "ment's ", "what ", "I'm ", "think", "ing ", "of", "", "\n\r",
	"You ", "would", "n't ", "get ", "this ", "from ", "any ", "oth", "er ", "guy", "\n\r",
	"I ", "just ", "wan", "na ", "tell ", "you ", "how ", "I'm ", "feel", "ing", "\n\r",
	"Got", "ta ", "make ", "you ", "", "un", "der", "stand\n\r\n\r"
};*/

const struct note tune_rickroll_chorus[] = {
	{NOTE_AS4, 100, 50}, {NOTE_AS4, 100, 50}, {NOTE_GS4, 100, 50}, {NOTE_GS4, 100, 50},
	{NOTE_F5, 300, 50}, {NOTE_F5, 300, 50}, {NOTE_DS5, 600, 50}, {NOTE_AS4, 100, 50},
	{NOTE_AS4, 100, 50}, {NOTE_GS4, 100, 50}, {NOTE_GS4, 100, 50}, {NOTE_DS5, 300, 50},
	{NOTE_DS5, 300, 50}, {NOTE_CS5, 300, 50}, {NOTE_C5, 100, 50}, {NOTE_AS4, 200, 50},
	{NOTE_CS5, 100, 50}, {NOTE_CS5, 100, 50}, {NOTE_CS5, 100, 50}, {NOTE_CS5, 100, 50},
	{NOTE_CS5, 300, 50}, {NOTE_DS5, 300, 50}, {NOTE_C5, 300, 50}, {NOTE_AS4, 100, 50},
	{NOTE_GS4, 200, 50}, {NOTE_GS4, 200, 50}, {NOTE_GS4, 200, 50}, {NOTE_DS5, 400, 50},
	{NOTE_CS5, 800, 50}, {NOTE_AS4, 100, 50}, {NOTE_AS4, 100, 50}, {NOTE_GS4, 100, 50},
	{NOTE_GS4, 100, 50}, {NOTE_F5, 300, 50}, {NOTE_F5, 300, 50}, {NOTE_DS5, 600, 50},
	{NOTE_AS4, 100, 50}, {NOTE_AS4, 100, 50}, {NOTE_GS4, 100, 50}, {NOTE_GS4, 100, 50},
	{NOTE_GS5, 300, 50}, {NOTE_C5, 300, 50}, {NOTE_CS5, 300, 50}, {NOTE_C5, 100, 50},
	{NOTE_AS4, 200, 50}, {NOTE_CS5, 100, 50}, {NOTE_CS5, 100, 50}, {NOTE_CS5, 100, 50},
	{NOTE_CS5, 100, 50}, {NOTE_CS5, 300, 50}, {NOTE_DS5, 300, 50}, {NOTE_C5, 300, 50},
	{NOTE_AS4, 100, 50}, {NOTE_GS4, 200, 50}, {NOTE_REST, 200, 50}, {NOTE_GS4, 200, 50},
	{NOTE_DS5, 400, 50}, {NOTE_CS5, 800, 50}, {NOTE_REST, 400, 50}
};

const char* tune_rickroll_chorus_lyrics[] = {
	"Ne", "ver ", "gon", "na ", "give ", "you ", "up", "\n\r",
	"Ne", "ver ", "gon", "na ", "let ", "you ", "down", "", "\n\r",
	"Ne", "ver ", "gon", "na ", "run ", "a", "round ", "and ", "de", "sert ", "you", "\n\r",
	"Ne", "ver ", "gon", "na ", "make ", "you ", "cry", "", "\n\r",
	"Ne", "ver ", "gon", "na ", "say ", "good", "bye ", "", "\n\r",
	"Ne", "ver ", "gon", "na ", "tell ", "a ", "lie ", "", "and ", "hurt ", "you", "\n\r\n\r"
};

// ASCII art for generator
#define ART_WIDTH   67
#define ART_LINES   35
const char ART_DATA[] = {
25, 17, 25, 19, 29, 19, 16, 35, 16, 13, 41, 13, 10, 19,         \
10, 18, 10, 8, 17, 18, 16, 8, 7, 16, 22, 15, 7, 5, 16,          \
26, 15, 5, 4, 16, 7, 5, 6, 5, 5, 15, 4, 3, 6, 3, 7, 7,          \
7, 4, 7, 5, 15, 3, 2, 6, 6, 5, 8, 5, 6, 5, 7, 7, 3, 5,          \
2, 1, 6, 7, 5, 31, 5, 7, 4, 1, 1, 7, 5, 6, 3, 4, 17, 4,         \
3, 5, 7, 4, 1, 0, 5, 11, 3, 4, 2, 19, 2, 4, 6, 7, 4, 0,         \
4, 20, 2, 17, 2, 4, 3, 13, 2, 0, 5, 5, 3, 12, 2, 15, 2,         \
21, 2, 0, 16, 11, 3, 9, 3, 13, 4, 5, 3, 0, 20, 10, 9, 12,       \
16, 0, 24, 23, 20, 0, 27, 11, 2, 3, 24, 0, 28, 1, 2, 10,        \
26, 1, 23, 8, 2, 11, 21, 1, 1, 19, 11, 7, 10, 18, 1, 2,         \
6, 5, 3, 11, 14, 11, 2, 5, 6, 2, 3, 5, 15, 22, 15, 4, 3,        \
4, 4, 11, 29, 11, 4, 4, 5, 7, 4, 35, 5, 6, 5, 7, 4, 6, 34,      \
5, 4, 7, 8, 3, 6, 34, 5, 3, 8, 10, 3, 2, 42, 10, 13, 41,        \
13, 16, 35, 16, 19, 29, 19, 25, 17, 25                          \
};

// Alternate ASCII art
/*#define ART_WIDTH   60
#define ART_LINES   36
const char ART_DATA[] = {                                       \
60, 1, 12, 26, 9, 12, 3, 8, 24, 17, 8, 4, 6, 23, 21, 6,         \
4, 6, 22, 12, 5, 6, 5, 4, 6, 21, 11, 8, 6, 4, 4, 6, 21,         \
10, 10, 5, 4, 4, 6, 21, 9, 11, 5, 4, 4, 6, 21, 8, 11, 6,        \
4, 4, 6, 21, 7, 11, 7, 4, 4, 6, 21, 6, 11, 8, 4, 4, 6,          \
19, 1, 1, 5, 11, 9, 4, 4, 6, 19, 1, 1, 5, 10, 10, 4, 4,         \
6, 18, 2, 1, 6, 8, 11, 4, 4, 6, 17, 3, 1, 7, 5, 13, 4, 4,       \
6, 15, 5, 2, 23, 5, 1, 29, 5, 17, 8, 1, 29, 9, 9, 12, 1,        \
13, 5, 40, 1, 1, 13, 5, 40, 1, 4, 6, 13, 3, 10, 6, 12, 5,       \
1, 5, 6, 11, 3, 11, 6, 14, 3, 1, 5, 6, 11, 3, 11, 6, 15,        \
2, 1, 6, 6, 9, 3, 12, 6, 16, 1, 1, 6, 6, 9, 3, 12, 6, 7,        \
1, 10, 7, 6, 7, 3, 13, 6, 6, 2, 10, 7, 6, 7, 3, 13, 14,         \
10, 8, 6, 5, 3, 14, 6, 6, 2, 10, 8, 6, 5, 3, 14, 6, 7, 1,       \
10, 9, 6, 3, 3, 15, 6, 16, 1, 1, 9, 6, 3, 3, 15, 6, 15,         \
2, 1, 10, 6, 1, 3, 16, 6, 14, 3, 1, 10, 10, 16, 6, 12, 5,       \
1, 11, 8, 13, 27, 1, 11, 8, 13, 27, 1, 60                       \
};*/


/****************************************************************************
 ********************* Function Prototypes **********************************
 ***************************************************************************/

// Badge
void DC27_GameInit(void);
void DC27_UpdateState(void);
void DC27_UpdateDisplay(void);
void DC27_UpdateFlags(bool);
int DC27_IncrementFlag(void);
void DC27_PrintBadgeType(badge_type_t);
void DC27_PrintState(void);
void DC27_PrintPacket(struct packet_of_infamy);
void DC27_ProcessPacket(void);
void DC27_InteractiveMode(void);
void DC27_ASCIIArt(uint8_t *);

// I2C
bool I2C_ReadRegister(I2C_Type *, uint8_t, uint8_t, uint8_t *, uint32_t);
bool I2C_WriteRegister(I2C_Type *, uint8_t , uint8_t , uint8_t);
bool I2C_ReadBulk(I2C_Type *, uint8_t, uint8_t *, uint32_t);
bool I2C_WriteBulk(I2C_Type *, uint8_t , uint8_t *, uint32_t);
void I2C_ReleaseBus(void);

// LED Driver
int KL_Setup_LP5569(void);
void LP5569_SetLED(unsigned char, unsigned char);
void LP5569_SetLED_AllOn(void);
void LP5569_SetLED_AllOff(void);
void LP5569_SetLED_D(unsigned char);
void LP5569_SetLED_E(unsigned char);
void LP5569_SetLED_F(unsigned char);
void LP5569_SetLED_C(unsigned char);
void LP5569_SetLED_O(unsigned char);
void LP5569_SetLED_N(unsigned char);
void LP5569_RampLED(badge_state_t);

// NFMI Radio
int KL_Setup_NXH2261(void);
int KL_Program_NXH2261(const uint32_t, const unsigned char *);
int KL_LoadImage_NXH2261(uint32_t, const uint8_t *, uint32_t);
int KL_GetPacket_NXH2261(struct packet_of_infamy *);
int KL_UpdatePacket_NXH2261(struct packet_of_infamy);
void KL_Reset_NXH2261(void);

// Piezo/PWM
void KL_Piezo(uint32_t, uint32_t, uint8_t);
void KL_Piezo_1Up(void);
void KL_Piezo_RickRoll(void);

// Utilities
unsigned char Get_Random_Byte(void);
void Print_Bits(uint8_t);
void Reorder_Array(uint8_t *, uint8_t *, uint8_t);
void SysTick_DelayTicks(uint32_t);

int KL_Flash_Init(void);
int KL_Flash_Write(uint32_t);
void KL_Flash_Read(uint32_t *);
bool KL_Check_RX(void);
void KL_Sleep(void);
void KL_Error(bool, bool);


/****************************************************************************
 ************************** Functions ***************************************
 ***************************************************************************/

/*
 * @brief   Application entry point.
 */
int main(void)
{
    static int b = 1;

  	// Initialize board hardware
	BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();

    // Initialize host console
	DbgConsole_Init(UART2_BASE, UART2_config.baudRate_Bps, DEBUG_CONSOLE_DEVICE_TYPE_UART, UART2_CLOCK_SOURCE);

	// Disable LPUART interrupts during power-up to avoid receiving data before we're ready
	DisableIRQ(LPUART0_SERIAL_RX_TX_IRQN);

	// Other initialization
    SMC_SetPowerModeProtection(SMC, kSMC_AllowPowerModeAll);	// Configure power mode protection settings
    CLOCK_SetClkOutClock(0); 					// Disable CLKOUT on power-up (used for NXH2261 calibration only)
	SysTick_Config(SystemCoreClock / 1000U); 	// Set systick reload value to generate 1ms interrupt (for delay)
	SysTick_DelayTicks(1000);					// Start-up delay
	I2C_ReleaseBus();

	// Send messages to console
	PRINTF(msg_welcome);
	PRINTF(msg_version);
	PRINTF(msg_init_start);

	// Display KL27 unique ID
	// https://community.nxp.com/thread/309453
	PRINTF("[*] MKL27Z64 Unique Identifier: ");
	PRINTF("0x%08X%08X%08X [32-bit: ", SIM->UIDMH, SIM->UIDML, SIM->UIDL);
	uint32_t idCodeShort = (SIM->UIDMH ^ SIM->UIDML ^ SIM->UIDL ^ SIM->SDID); // condense ID into 32-bit value (collisions may occur between units)
	PRINTF("0x%08X]\n\r", idCodeShort);
    PRINTF("[*] Core Clock = %dHz\n\r", CLOCK_GetFreq(kCLOCK_CoreSysClk));

    PRINTF("[*] Initializing Flash Memory");
    if (KL_Flash_Init())
    	PRINTF("...Error!\n\r");

    // Display badge type and configure default badge-specific parameters
    // Different LED colors have different Vf, which affects brightness
    // We want all LEDs to visually appear at the same brightness regardless of color
    PRINTF("[*] Badge Type = ");
    DC27_PrintBadgeType(badge_type);
    switch(badge_type)
    {
    	case HUMAN:
    		LP5569_Control = LED_CONTROL_WHITE;  // Control Register
    		LP5569_Current = LED_CURRENT_WHITE;  // Current Control
    		LP5569_PWM = LED_PWM_WHITE;  		 // PWM Duty Cycle
    		break;
    	case GOON:
    		LP5569_Control = LED_CONTROL_RED;
    		LP5569_Current = LED_CURRENT_RED;
    		LP5569_PWM = LED_PWM_RED;
    		break;
    	case SPEAKER:
    		LP5569_Control = LED_CONTROL_BLUE;
    		LP5569_Current = LED_CURRENT_BLUE;
    		LP5569_PWM = LED_PWM_BLUE;
    		break;
    	case VENDOR:
    		LP5569_Control = LED_CONTROL_PURPLE;
    		LP5569_Current = LED_CURRENT_PURPLE;
    		LP5569_PWM = LED_PWM_PURPLE;
    		break;
    	case PRESS:
    		LP5569_Control = LED_CONTROL_GREEN;
    		LP5569_Current = LED_CURRENT_GREEN;
    		LP5569_PWM = LED_PWM_GREEN;
    		break;
    	case VILLAGE:
    		LP5569_Control = LED_CONTROL_ORANGE;
    		LP5569_Current = LED_CURRENT_ORANGE;
    		LP5569_PWM = LED_PWM_ORANGE;
    		break;
    	case CONTEST:
    		LP5569_Control = LED_CONTROL_WHITE;
    		LP5569_Current = LED_CURRENT_WHITE;
    		LP5569_PWM = LED_PWM_WHITE;
    		break;
    	case ARTIST:
    		LP5569_Control = LED_CONTROL_WHITE;
    		LP5569_Current = LED_CURRENT_WHITE;
    		LP5569_PWM = LED_PWM_WHITE;
    		break;
    	case CFP:
    		LP5569_Control = LED_CONTROL_WHITE;
    		LP5569_Current = LED_CURRENT_WHITE;
    		LP5569_PWM = LED_PWM_WHITE;
    		break;
    	case UBER:
    		LP5569_Control = LED_CONTROL_WHITE;
    		LP5569_Current = LED_CURRENT_WHITE;
    		LP5569_PWM = LED_PWM_WHITE;
    		break;
    	default:
    		LP5569_Control = LED_CONTROL_WHITE;
    		LP5569_Current = LED_CURRENT_WHITE;
    		LP5569_PWM = LED_PWM_WHITE;
    }

	DC27_GameInit();

    PRINTF("[*] Magic Token = ");
#ifdef __BADGE_MAGIC
	PRINTF("True\n\r");		// magic token (1 = enabled)
#else
	PRINTF("False\n\r");	// magic token (0 = disabled)
#endif

    PRINTF("[*] Testing Piezo...");
    KL_Piezo_1Up();
	PRINTF("Done!\n\r");
    SysTick_DelayTicks(250);

    PRINTF("[*] Configuring LED Driver...");
    if (!KL_Setup_LP5569())
    {
    	LP5569_SetLED_AllOn();
        PRINTF("Done!\n\r");
    }
    else
    {
        SysTick_DelayTicks(100);
        if (KL_Setup_LP5569())	// Try again...
        {
        	KL_Error(true, false);  // Beep the piezo to indicate a failure
        	PRINTF("Error!\n\r");
        }
    }

    // Now that the system is up and running, enable UART interrupts from the NXH
    EnableIRQ(LPUART0_SERIAL_RX_TX_IRQN);

    PRINTF("[*] Configuring NFMI Radio\n\r");
    if (KL_Setup_NXH2261())
    {
    	SysTick_DelayTicks(100);
    	if (KL_Setup_NXH2261())  // Try again...
    	{
    		KL_Error(false, true);  // Blink the LEDs to indicate a failure
    	}
    }

	// Craft data packet for radio to transmit
	nxhTxPacket.uid = idCodeShort;			// unique ID
	nxhTxPacket.type = (uint8_t)badge_type;	// badge type
#ifdef __BADGE_MAGIC
	nxhTxPacket.magic = true; 				// magic token (1 = enabled)
#else
	nxhTxPacket.magic = false; 				// magic token (0 = disabled)
#endif
    nxhTxPacket.flags = game_flags;			// game flags (packed, MSB unused)
	nxhTxPacket.unused = 0; 				// unused

    if (KL_UpdatePacket_NXH2261(nxhTxPacket))  // load transmit packet into the NXH2261
    {
        if (KL_UpdatePacket_NXH2261(nxhTxPacket))
    		PRINTF(msg_nfmi_packet_err);
    }

	g_oldRx = KL_Check_RX();  // set current state of KL_RX (if USB-to-serial adapter is connected)

	LP5569_SetLED(0, 0); // Update start-up progress via LEDs
	LP5569_SetLED(5, 0);

	PRINTF(msg_init_complete);

	if (badge_state == COMPLETE)  // if the quest is done, play a friendly tune
	{
		SysTick_DelayTicks(100);
		PRINTF("\n\r");
		KL_Piezo_RickRoll();
		SysTick_DelayTicks(500);
	}

	while(1)
    {
    	g_newRx = KL_Check_RX();
    	if (g_newRx && (!g_oldRx || b))	// if USB-to-Serial adapter has been plugged in, KL_RX pin will go HIGH
    	{
    		PRINTF("[*] USB-to-Serial Adapter Detected\n\r");
    		if (!b) GETCHAR(); // if adapter is plugged in while the system is already active, drop first character
    		DC27_InteractiveMode();
    		b = 0; // if we're here for the first time after power-up
    	}
    	g_oldRx = g_newRx;

    	if (badge_state != COMPLETE) // do this if all badge tasks have not been completed
    	{
        	// enter VLPS (Very Low Power Sleep)
        	// MCU will wake up on NXH_DETECT external interrupt (when NXH successfully receives a data packet)
    		// or if USB-to-serial adapter is connected
        	PRINTF("[*] Sleeping...");
        	DbgConsole_Flush();	// wait for TX buffer to empty
        	PORT_SetPinInterruptConfig(BOARD_INITPINS_KL_RX_PORT, BOARD_INITPINS_KL_RX_PIN, kPORT_InterruptRisingEdge);
        	LPTMR_SetTimerPeriod(LPTMR0_PERIPHERAL, LPTMR0_TICKS);	// Set time to sleep between LED update/heartbeat

        	KL_Sleep();

            PRINTF("Awake!\n\r");
    	}

    	if (g_lptmrFlag && badge_state != COMPLETE) 	// we must have woken up from the timer
    	{
    		if (badge_state == ATTRACT)
    			DC27_UpdateDisplay();

        	g_lptmrFlag = false;
    	}

    	// Check badge state and update if needed
    	DisableIRQ(LPUART0_SERIAL_RX_TX_IRQN);
    	DC27_UpdateState();
        EnableIRQ(LPUART0_SERIAL_RX_TX_IRQN);
    }

    return 0; // We should never reach here
}

/**************************************************************/

void DC27_GameInit(void)	// Initialize DC27 badge game-related items
{
	uint32_t data;

#ifdef __BADGE_MAGIC	// for magic token, skip attract mode to save battery
	badge_state = D;
#else
	badge_state = ATTRACT;
	attract_state = D; // used for cycling through LED states during attract mode
#endif

	// Configure game flags (for development/debugging purposes)
	//game_flags = 0;				// Clear all
	//game_flags = 0b00111111;		// State N
	//game_flags = FLAG_ALL_MASK; 	// Set all
	//DC27_UpdateFlags(false);

#ifdef __BADGE_MAGIC	// reset game flags, since they're not being used
	game_flags = 0;
	DC27_UpdateFlags(false);
#endif

	// read game flags from non-volatile Flash (persists between power cycles)
    KL_Flash_Read(&data);
    if (data == 0xFFFFFFFF)		// on first use/power-up of the badge, the flash area will be uninitialized
    {
    	game_flags = 0;				// clear flags
    	DC27_UpdateFlags(false);	// write back to flash
    }
    else
    {
    	game_flags = (uint8_t)data;
    	if ((game_flags & FLAG_ALL_MASK) == FLAG_ALL_MASK)	 // if quest is complete...
    	{
    		badge_state = COMPLETE;			// go straight to sparkle mode
    	}
    	else
    	{
    		game_flags &= ~FLAG_0_MASK;		// clear flag (require communication with another person on power-up)
    	}
    }

	DC27_PrintState();
}

/**************************************************************/

void DC27_UpdateState(void)
{
    static int i, j;
	uint8_t ch;

	switch (badge_state)
	{
		default:
	    case ATTRACT: // Attract mode: Cycle through D, E, F, C, O, N LED states
	    	if (g_nxhDetect && !KL_GetPacket_NXH2261(&nxhRxPacket)) // if we have packet(s) in the receive buffer
    		{
				DC27_ProcessPacket();	// process the first one
    		}
	    	while (!KL_GetPacket_NXH2261(&nxhRxPacket)){}; // clear the rest
	    	g_nxhDetect = false;

	    	if ((game_flags & FLAG_0_MASK) == 1) // Communication with anyone
	        {
	        	// set badge state accordingly based on game flags settings
	        	// flags 1-5 can happen in any order, so badge state is determined
	        	// by how many bits have been set so far
            	ch = 0;
            	j = game_flags >> 1;
            	for (i = 0; i < 5; ++i)
            	{
            		ch += (j & 0x01);
            		j >>= 1;
            	}

            	switch (ch)
				{
					case 0:
						badge_state = D;
						break;
					case 1:
						badge_state = E;
						break;
					case 2:
						badge_state = F;
						break;
					case 3:
						badge_state = C;
						break;
					case 4:
						badge_state = O;
						break;
					case 5:
						badge_state = N;
						break;
				}

    			DC27_UpdateDisplay();
    			KL_Piezo_1Up();
            	DC27_PrintState();
        		SysTick_DelayTicks(500);
        		LP5569_SetLED_AllOff();
	   			DC27_UpdateFlags(true);
	        }
	        break;

	   	case D:
	    	if (g_nxhDetect && !KL_GetPacket_NXH2261(&nxhRxPacket)) // if we have packet(s) in the receive buffer
    		{
				DC27_ProcessPacket();	// process the first one
#ifdef __BADGE_MAGIC
				LP5569_SetLED_AllOn();
#else
				DC27_UpdateDisplay();
#endif
				SysTick_DelayTicks(500);
				LP5569_SetLED_AllOff();
    		}
	    	while (!KL_GetPacket_NXH2261(&nxhRxPacket)){}; // clear the rest
	    	g_nxhDetect = false;

#ifndef __BADGE_MAGIC  // magic token stays in this state
	    	if (nxhRxPacket.magic == true) // if we've received data from a magic token
	    	{
		   		if (DC27_IncrementFlag())   // if we've received a flag we don't already have, move to the next state
		   		{
					SysTick_DelayTicks(500);
			   		badge_state = E;
	    			DC27_UpdateDisplay();
	    			KL_Piezo_1Up();
	            	DC27_PrintState();
	        		SysTick_DelayTicks(500);
	        		LP5569_SetLED_AllOff();
		   			DC27_UpdateFlags(true);
		   		}
	    	}
#endif
	   		break;

	   	case E:
	    	if (g_nxhDetect && !KL_GetPacket_NXH2261(&nxhRxPacket)) // if we have packet(s) in the receive buffer
    		{
				DC27_ProcessPacket();	// process the first one
				DC27_UpdateDisplay();
				SysTick_DelayTicks(500);
				LP5569_SetLED_AllOff();
    		}
	    	while (!KL_GetPacket_NXH2261(&nxhRxPacket)){}; // clear the rest
	    	g_nxhDetect = false;

	    	if (nxhRxPacket.magic == true) // if we've received data from a magic token
	    	{
		   		if (DC27_IncrementFlag())   // if we've received a flag we don't already have, move to the next state
		   		{
					SysTick_DelayTicks(500);
			   		badge_state = F;
	    			DC27_UpdateDisplay();
	    			KL_Piezo_1Up();
	            	DC27_PrintState();
	        		SysTick_DelayTicks(500);
	        		LP5569_SetLED_AllOff();
		   			DC27_UpdateFlags(true);
		   		}
	    	}
	        break;

       	case F:
	    	if (g_nxhDetect && !KL_GetPacket_NXH2261(&nxhRxPacket)) // if we have packet(s) in the receive buffer
    		{
				DC27_ProcessPacket();	// process the first one
				DC27_UpdateDisplay();
				SysTick_DelayTicks(500);
				LP5569_SetLED_AllOff();
    		}
	    	while (!KL_GetPacket_NXH2261(&nxhRxPacket)){}; // clear the rest
	    	g_nxhDetect = false;

	    	if (nxhRxPacket.magic == true) // if we've received data from a magic token
	    	{
		   		if (DC27_IncrementFlag())   // if we've received a flag we don't already have, move to the next state
		   		{
					SysTick_DelayTicks(500);
			   		badge_state = C;
	    			DC27_UpdateDisplay();
	    			KL_Piezo_1Up();
	            	DC27_PrintState();
	        		SysTick_DelayTicks(500);
	        		LP5569_SetLED_AllOff();
		   			DC27_UpdateFlags(true);
		   		}
	    	}
       		break;

       	case C:
	    	if (g_nxhDetect && !KL_GetPacket_NXH2261(&nxhRxPacket)) // if we have packet(s) in the receive buffer
    		{
				DC27_ProcessPacket();	// process the first one
				DC27_UpdateDisplay();
				SysTick_DelayTicks(500);
				LP5569_SetLED_AllOff();
    		}
	    	while (!KL_GetPacket_NXH2261(&nxhRxPacket)){}; // clear the rest
	    	g_nxhDetect = false;

	    	if (nxhRxPacket.magic == true) // if we've received data from a magic token
	    	{
		   		if (DC27_IncrementFlag())   // if we've received a flag we don't already have, move to the next state
		   		{
					SysTick_DelayTicks(500);
			   		badge_state = O;
	    			DC27_UpdateDisplay();
	    			KL_Piezo_1Up();
	            	DC27_PrintState();
	        		SysTick_DelayTicks(500);
	        		LP5569_SetLED_AllOff();
		   			DC27_UpdateFlags(true);
		   		}
	    	}
       		break;

       	case O:
	    	if (g_nxhDetect && !KL_GetPacket_NXH2261(&nxhRxPacket)) // if we have packet(s) in the receive buffer
    		{
				DC27_ProcessPacket();	// process the first one
				DC27_UpdateDisplay();
				SysTick_DelayTicks(500);
				LP5569_SetLED_AllOff();
    		}
	    	while (!KL_GetPacket_NXH2261(&nxhRxPacket)){}; // clear the rest
	    	g_nxhDetect = false;

	    	if (nxhRxPacket.magic == true) // if we've received data from a magic token
	    	{
		   		if (DC27_IncrementFlag())   // if we've received a flag we don't already have, move to the next state
		   		{
					SysTick_DelayTicks(500);
			   		badge_state = N;
	    			DC27_UpdateDisplay();
	    			KL_Piezo_1Up();
	            	DC27_PrintState();
	        		SysTick_DelayTicks(500);
	        		LP5569_SetLED_AllOff();
		   			DC27_UpdateFlags(true);
		       		group_flags = 0;
		   		}
	    	}
       		break;

       	case N: // Group chat (all 6 gemstone colors: Human/Contest/Artist/CFP/Uber + Goon + Speaker + Vendor + Press + Village)
	    	if (g_nxhDetect && !KL_GetPacket_NXH2261(&nxhRxPacket)) // if we have packet(s) in the receive buffer
    		{
				DC27_ProcessPacket();	// process the first one
				DC27_UpdateDisplay();
				SysTick_DelayTicks(500);
				LP5569_SetLED_AllOff();
    		}
	    	while (!KL_GetPacket_NXH2261(&nxhRxPacket)){}; // clear the rest
	    	g_nxhDetect = false;

			switch (nxhRxPacket.type)
			{
				case HUMAN:
				case CONTEST:
				case ARTIST:
				case CFP:
				case UBER:
					group_flags |= FLAG_0_MASK;
					break;
				case GOON:
					group_flags |= FLAG_1_MASK;
					break;
				case SPEAKER:
					group_flags |= FLAG_2_MASK;
					break;
				case VENDOR:
					group_flags |= FLAG_3_MASK;
					break;
				case PRESS:
					group_flags |= FLAG_4_MASK;
					break;
				case VILLAGE:
					group_flags |= FLAG_5_MASK;
					break;
			}

       		// win!
	    	if ((group_flags & GROUP_ALL_MASK) == GROUP_ALL_MASK)
	    	{
				game_flags |= FLAG_6_MASK;
				badge_state = COMPLETE;
				LP5569_SetLED_AllOff();
				SysTick_DelayTicks(500);
				KL_Piezo_RickRoll();
				DC27_PrintState();
				DC27_UpdateFlags(true);
				SysTick_DelayTicks(1500);
	    	}
    		break;

   	    case COMPLETE: // Sparkle mode
   	    	DC27_UpdateDisplay();
   	    	LPTMR_SetTimerPeriod(LPTMR0_PERIPHERAL, LED_SPARKLE_WAIT_DELAY);	// Set time to sleep between LED updates
	    	while (!KL_GetPacket_NXH2261(&nxhRxPacket)){}; // clear any packets from buffer so we don't overflow
	    	g_nxhDetect = false;
   	    	KL_Sleep(); // Go to sleep until our delay period is complete
	    	break;
	}
}

/**************************************************************/

void DC27_UpdateDisplay(void)	// update LEDs based on current state
{
	static volatile int j;

    switch (badge_state)
    {
    	default:
    	case ATTRACT: // Attract mode: Cycle through D, E, F, C, O, N LED states
    		switch (attract_state)
    		{
    			case D:
    	    		LP5569_RampLED(D);
    	    		attract_state = E;
    				break;
    			case E:
    	    		LP5569_RampLED(E);
    	    		attract_state = F;
    				break;
    			case F:
    	    		LP5569_RampLED(F);
    	    		attract_state = C;
    				break;
    			case C:
    	    		LP5569_RampLED(C);
    	    		attract_state = O;
    				break;
    			case O:
    	    		LP5569_RampLED(O);
    	    		attract_state = N;
    				break;
    			case N:
    	    		LP5569_RampLED(N);
    	    		attract_state = D;
    				break;
    			case ATTRACT:	// unused states
    			case COMPLETE:
    			default:
    				attract_state = D;
    				break;
    		}
    		break;

    	case D:
    		LP5569_SetLED_D(LP5569_PWM);
    		break;

    	case E:
    		LP5569_SetLED_E(LP5569_PWM);
    		break;

    	case F:
    		LP5569_SetLED_F(LP5569_PWM);
    		break;

    	case C:
    		LP5569_SetLED_C(LP5569_PWM);
    		break;

    	case O:
    		LP5569_SetLED_O(LP5569_PWM);
    		break;

    	case N:
    		LP5569_SetLED_N(LP5569_PWM);
    		break;

    	case COMPLETE:
    		for (j = 0; j <= LP5569_PWM; j++)  // ramp up to maximum defined brightness
    		{
    			LP5569_SetLED(1, j);
    			LP5569_SetLED(4, j);

        		SysTick_DelayTicks(LED_SPARKLE_FADE_DELAY);
    		}

    		SysTick_DelayTicks(LED_SPARKLE_ON_DELAY);

       		LP5569_SetLED_AllOff(); // clear display
    		LP5569_SetLED(0, LP5569_PWM);
    	    LP5569_SetLED(3, LP5569_PWM);
       		SysTick_DelayTicks(LED_SPARKLE_ON_DELAY);

       		LP5569_SetLED_AllOff(); // clear display
       		LP5569_SetLED(1, LP5569_PWM);
       		LP5569_SetLED(4, LP5569_PWM);
       		SysTick_DelayTicks(LED_SPARKLE_ON_DELAY);

       		LP5569_SetLED_AllOff(); // clear display
    		LP5569_SetLED(2, LP5569_PWM);
    	    LP5569_SetLED(5, LP5569_PWM);
       		SysTick_DelayTicks(LED_SPARKLE_ON_DELAY);

       		LP5569_SetLED_AllOff(); // clear display
    		LP5569_SetLED(1, LP5569_PWM);
    	    LP5569_SetLED(4, LP5569_PWM);
       		SysTick_DelayTicks(LED_SPARKLE_ON_DELAY);

    		for (j = LP5569_PWM; j >= 0; j--)  // ramp down from maximum defined brightness
    		{
    			LP5569_SetLED(1, j);
    			LP5569_SetLED(4, j);

        		SysTick_DelayTicks(LED_SPARKLE_FADE_DELAY);
    		}

    		break;
    }
}

/**************************************************************/

void DC27_UpdateFlags(bool updateNXH)
{
	// write game flags to non-volatile Flash (persists between power cycles)
	if (KL_Flash_Write((uint32_t)game_flags))
	{
		if (KL_Flash_Write((uint32_t)game_flags))
		{
			KL_Error(true, false);  // Beep the piezo to indicate a failure
			PRINTF("[*] Flash Write Error!\n\r");
		}
	}

	nxhTxPacket.flags = game_flags;	 // game flags (packed, MSB unused)

	if (updateNXH)
	{
        EnableIRQ(LPUART0_SERIAL_RX_TX_IRQN);
	    if (KL_UpdatePacket_NXH2261(nxhTxPacket))  // load updated transmit packet into the NXH2261
	    {
	        if (KL_UpdatePacket_NXH2261(nxhTxPacket))
	    		PRINTF(msg_nfmi_packet_err);
	    }
        DisableIRQ(LPUART0_SERIAL_RX_TX_IRQN);
	}
}

/**************************************************************/

int DC27_IncrementFlag(void)
{
	switch (nxhRxPacket.type)
	{
		case SPEAKER:
			if ((game_flags & FLAG_1_MASK) == 0)
			{
				game_flags |= FLAG_1_MASK;
				return 1;
			}
			break;

		case VILLAGE:
			if ((game_flags & FLAG_2_MASK) == 0)
			{
				game_flags |= FLAG_2_MASK;
				return 1;
			}
			break;

		case CONTEST:
			if ((game_flags & FLAG_3_MASK) == 0)
			{
				game_flags |= FLAG_3_MASK;
				return 1;
			}
			break;

		case ARTIST:
			if ((game_flags & FLAG_4_MASK) == 0)
			{
				game_flags |= FLAG_4_MASK;
				return 1;
			}
			break;

		case GOON:
			if ((game_flags & FLAG_5_MASK) == 0)
			{
				game_flags |= FLAG_5_MASK;
				return 1;
			}
			break;
	}

	return 0;
}

/**************************************************************/

void DC27_PrintBadgeType(badge_type_t badge)	// print badge type to console
{
	switch(badge)
	{
	   	case HUMAN:
	   		PRINTF("Human");
	   		break;
	   	case GOON:
	   		PRINTF("Goon");
	   		break;
	   	case SPEAKER:
	   		PRINTF("Speaker");
	   		break;
	   	case VENDOR:
	   		PRINTF("Vendor");
	   		break;
	   	case PRESS:
	   		PRINTF("Press");
	   		break;
	   	case VILLAGE:
	   		PRINTF("Village");
	   		break;
	   	case CONTEST:
	   		PRINTF("Contest");
	   		break;
	   	case ARTIST:
	   		PRINTF("Artist");
	   		break;
	   	case CFP:
	   		PRINTF("CFP");
	   		break;
	   	case UBER:
	   		PRINTF("Uber");
	   		break;
	   	default:
	   		PRINTF("Unknown");
	}

	PRINTF("\n\r");
}

/**************************************************************/

void DC27_PrintState(void)	// print current state and game flags to console
{
	PRINTF("[*] Badge State = ");

    switch (badge_state)
    {
    	default:
    	case ATTRACT:
    		PRINTF("Attract");
    		break;

    	case D:
    		PRINTF("D");
    		break;

    	case E:
    		PRINTF("E");
    		break;

    	case F:
    		PRINTF("F");
    		break;

    	case C:
    		PRINTF("C");
    		break;

    	case O:
    		PRINTF("O");
    		break;

    	case N:
    		PRINTF("N");
    		break;

    	case COMPLETE:
    		PRINTF("Hax0r");
    		break;
    }

	PRINTF("\n\r");

    PRINTF("[*] Game Flags = ");
    Print_Bits(game_flags);
    PRINTF("\n\r");
}

/**************************************************************/

void DC27_PrintPacket(struct packet_of_infamy packet)
{
	// display hex data from data packet struct
	const unsigned char *buffer = (unsigned char*)&packet;

	PRINTF("0x%02X%02X%02X%02X%02X%02X%02X%02X\n\r", buffer[3], buffer[2], buffer[1], buffer[0], buffer[4], buffer[5], buffer[6], buffer[7]);
	PRINTF("-> Unique ID: 0x%08X\n\r", packet.uid);
	PRINTF("-> Badge Type: ");
	DC27_PrintBadgeType(packet.type);
	PRINTF("-> Magic Token: ");
	if (packet.magic != 0)
		PRINTF("Yes");
	else
		PRINTF("No");
	PRINTF("\n\r-> Game Flags: ");
	Print_Bits(packet.flags); // MSB unused
    PRINTF("\n\r");
}

/**************************************************************/

void DC27_ProcessPacket(void)
{
	DC27_PrintPacket(nxhRxPacket);   // print packet structure to debug console
	PRINTF("\n\r");

	if ((game_flags & FLAG_0_MASK) == 0) // on first read, set game flag
	{
		game_flags |= FLAG_0_MASK;
	}
}

/**************************************************************/

void DC27_InteractiveMode(void)
{
	size_t i;
	uint8_t ch, inputString[CONSOLE_RCVBUF_SIZE];
	uint32_t len;

	PRINTF(msg_interactive_mode);

	while(1)
	{
		PRINTF(command_prompt);
		len = 0;
		while(1)
		{
		  ch = GETCHAR();			// get character from the user (blocking)
		  inputString[len] = ch;	// add to input buffer
		  len += 1;
		  if (ch == 24) // CAN (Ctrl-X)
		  {
			  PRINTF(msg_interactive_exit);
			  return;
		  }
		  else if (ch == '\n' || ch == '\r' || (len > CONSOLE_RCVBUF_SIZE - 1))  // take input until CR, LF, or maximum length received
		  {
			  inputString[len - 1] = '\0';
			  break;
		  }
		}

		len = strlen((char *)inputString);
		switch(inputString[0])
		{
			case 'T': 	// Display transmit packet
			case 't':
				if (len != 1) // if input string is longer than allowable for this command, ignore it
					dc27_invalid_cmd();
				else
				{
					DC27_PrintPacket(nxhTxPacket);  // print packet structure to debug console
				}
				break;

			case 'R':	// Receive data packet(s)
			case 'r':
				if (len != 1) // if input string is longer than allowable for this command, ignore it
					dc27_invalid_cmd();
				else
				{
					i = 0;
					while (KL_GetPacket_NXH2261(&nxhRxPacket))  // if there is no available packet...
					{
						if (i == 0)  // only print message one time
						{
							PRINTF("Waiting for Packet(s)...\n\r");
							i = 1;
						}

						g_nxhDetect = false;
						while (!g_nxhDetect)  // wait here until a new packet has been received
						{
							if (!KL_Check_RX())
							{
								PRINTF(msg_interactive_exit);
								return;		// exit interactive mode if USB-to-serial adapter is removed
							}
						}
						g_nxhDetect = false;
					}

					DC27_ProcessPacket();
					if (badge_state == ATTRACT || badge_state == COMPLETE)
					{
						LP5569_SetLED_AllOn();
					}
					else
					{
						DC27_UpdateDisplay();	// update LEDs based on current state
					}
					SysTick_DelayTicks(500);
					LP5569_SetLED_AllOff();

					// get any other packet(s) from the ring buffer
					while (!KL_GetPacket_NXH2261(&nxhRxPacket))
					{
						DC27_ProcessPacket();
						if (badge_state == ATTRACT || badge_state == COMPLETE)
						{
							LP5569_SetLED_AllOn();
						}
						else
						{
							DC27_UpdateDisplay();	// update LEDs based on current state
						}
						SysTick_DelayTicks(500);
						LP5569_SetLED_AllOff();
					}
				}
				break;

			case 'C':	// Clear game flags
			case 'c':
				if (len != 1) // if input string is longer than allowable for this command, ignore it
					dc27_invalid_cmd();
				else
				{
					PRINTF("Clear Game Flags? Are You Sure? [y/N] ");
					len = 0;
					while(1)
					{
						ch = GETCHAR();			// get character from the user (blocking)
						inputString[len] = ch;	// add to input buffer
					    len += 1;
					    if (ch == '\n' || ch == '\r' || (len > CONSOLE_RCVBUF_SIZE - 1))  // take input until CR, LF, or maximum length received
						{
					    	inputString[len - 1] = '\0';
							break;
						}
					}
					len = strlen((char *)inputString);
					if (len == 1 && (inputString[0] == 'Y' || inputString[0] == 'y'))
					{
						game_flags = 0;	// Clear game flags
						DC27_UpdateFlags(true);
						PRINTF("-> Game Flags: ");
						Print_Bits(game_flags); // MSB unused
					    PRINTF("\n\r");
					}
				}
				break;

			case '^':
				NVIC_SystemReset(); // System reset (does not return)
				break;

			case 'A':	// ASCII art generator
			case 'a':
				if (badge_state != COMPLETE)
					dc27_invalid_cmd();
				else if (len <= 2)
				{
					strcpy((char *)inputString, ART_DEFAULT);
					DC27_ASCIIArt(inputString); // use default string if none provided on the command line
				}
				else
					DC27_ASCIIArt(inputString + 2); // send user-defined string to the ASCII art generator
				break;

			case 'S': 	// Tone generator
			case 's':
				if (len < 5 || badge_state != COMPLETE)
					dc27_invalid_cmd();
				else
				{
					uint32_t freq = 0, duration = 0;
					sscanf((char *)(inputString + 2), "%d %d", &freq, &duration);
					KL_Piezo(freq, duration, 50);
				}
				break;

			case 'U':  // Update outgoing data packet
			case 'u':
				if (len < 18 || badge_state != COMPLETE)
					dc27_invalid_cmd();
				else
				{
					// convert all alphabetic characters to upper case
					for (i = 0; i < len; ++i)
					{
						if (inputString[i] >= 'a' && inputString[i] <= 'z')
							inputString[i] -= 0x20;
					}
					unsigned long data_low = strtoul((const char*)(inputString + 10), NULL, 16);
					inputString[10] = '\0';
					unsigned long data_high = strtoul((const char*)(inputString + 2), NULL, 16);
					PRINTF("0x%08X%08X\n\r", data_high, data_low);
					PRINTF("Update Transmit Packet? Are You Sure? [y/N] ");
					len = 0;
					while(1)
					{
						ch = GETCHAR();			// get character from the user (blocking)
						inputString[len] = ch;	// add to input buffer
					    len += 1;
					    if (ch == '\n' || ch == '\r' || (len > CONSOLE_RCVBUF_SIZE - 1))  // take input until CR, LF, or maximum length received
						{
					    	inputString[len - 1] = '\0';
							break;
						}
					}
					len = strlen((char *)inputString);
					if (len == 1 && (inputString[0] == 'Y' || inputString[0] == 'y'))
					{
						// Craft new data packet for radio to transmit
						nxhTxPacket.uid = data_high;					// unique ID
						nxhTxPacket.type = (data_low >> 24) & 0xFF;		// badge type
						nxhTxPacket.magic = (data_low >> 16) & 0xFF; 	// magic token (1 = enabled)
					    nxhTxPacket.flags = (data_low >> 8) & 0xFF;   	// game flags (packed, MSB unused)
						nxhTxPacket.unused = data_low & 0xFF;			// unused

					    if (KL_UpdatePacket_NXH2261(nxhTxPacket))  // load packet to the NXH2261
					    {
					        if (KL_UpdatePacket_NXH2261(nxhTxPacket))
					    		PRINTF(msg_nfmi_packet_err);
					    }
						else
							PRINTF("-> Done!\n\r");
					}
				}
				break;

			case 'H':	// Display menu
			case 'h':
			case '?':
				if (len != 1) // if input string is longer than allowable for this command, ignore it
					dc27_invalid_cmd();
				else
				{
					PRINTF(menu_banner);

					if (badge_state == COMPLETE)  // if badge tasks are complete, display additional menu items
						PRINTF(menu_banner_complete);
				}
				break;
			default:	// Command not recognized
				dc27_invalid_cmd();
				break;
		}
	}
}

/**************************************************************/

// ASCII art generator
// Inspired by:
// https://twitter.com/ef1j95/status/1101865167741173760
// http://www.vintage-basic.net/bcg/love.bas
// https://www.associationforpublicart.org/artwork/love/
void DC27_ASCIIArt(uint8_t *A)
{
	volatile uint32_t I, J, L, C, P, A1, ptr;
	uint8_t T[120];    // crafted line of text to print

	L = strlen((char *)A); // input text from user

	// craft a line based on the input text from user
	for (J = 0; J < (ART_WIDTH / L) + 1; J++)
	{
	    for (I = 0; I < L; I++) // repeat the input text for the entire line
	    {
	        T[(J * L) + I] = A[I];
	    }
	}

	C = 0;
	A1 = 0;
	ptr = 0;

	while(1)
	{
	    if (A1 == 0 || A1 > ART_WIDTH)  // print one line at a time
	    {
	        A1 = 1;         // current column count into the line
	        P = 0;          // flag to print message characters (true) or spaces (false)
	        C += 1;
	        if (C == ART_LINES)    // if we've reached the end of our artwork
	        {
		        PRINTF("\n\r");
	            return;  // done!
	        }
	        PRINTF("\n\r ");
	    }

	    A[0] = ART_DATA[ptr++];  // read from the data array (number of the next set of characters or spaces)
	    A1 += A[0];
	    if (P)
	    {
	        for (I = (A1 - A[0] - 1); I < A1 - 1; I++)
	        {
	            PRINTF("%c", T[I]);  // print character from the crafted message array
	        }
	        P = 0;
	    }
	    else
	    {
	        for (I = 0; I < A[0]; I++)
	        {
	            PRINTF(" "); // print space
	        }
	        P = 1;
	    }
	}
}

/**************************************************************/

int KL_Flash_Init(void)
{
	ftfx_security_state_t securityStatus = kFTFx_SecurityStateNotSecure;  // Memory protection status
	status_t result;    // Return code from each flash driver function

    // Clear structs
    memset(&s_flashDriver, 0, sizeof(flash_config_t));
    memset(&s_cacheDriver, 0, sizeof(ftfx_cache_config_t));

    // Setup flash driver
    result = FLASH_Init(&s_flashDriver);
    if (result != kStatus_FTFx_Success)
    {
    	return 1;
    }

    // Setup flash cache driver
    result = FTFx_CACHE_Init(&s_cacheDriver);
    if (result != kStatus_FTFx_Success)
    {
    	return 1;
    }

    // Check security status
    result = FLASH_GetSecurityState(&s_flashDriver, &securityStatus);
    if (result != kStatus_FTFx_Success)
    {
    	return 1;
    }

    // Only proceed with unprotected memory (we won't be able to read/write otherwise)
    if (securityStatus != kFTFx_SecurityStateNotSecure)
    {
    	return 1;
    }

    // Get flash properties
    FLASH_GetProperty(&s_flashDriver, kFLASH_PropertyPflash0BlockBaseAddr, &pflashBlockBase);
    FLASH_GetProperty(&s_flashDriver, kFLASH_PropertyPflash0TotalSize, &pflashTotalSize);
    FLASH_GetProperty(&s_flashDriver, kFLASH_PropertyPflash0SectorSize, &pflashSectorSize);

    // Print flash information to debug console
    PRINTF("\n\r-> Memory Size = %dKB [0x%X]\n\r", (pflashTotalSize / 1024), pflashTotalSize);
    PRINTF("-> Sector Size = %dKB [0x%X]\n\r", (pflashSectorSize / 1024), pflashSectorSize);

    return 0;
}

/**************************************************************/

// Erase and write a single Flash sector
int KL_Flash_Write(uint32_t data)
{
	status_t result;    	// Return code from each flash driver function
    uint32_t destAddress; 	// Base address of the target memory location
    uint32_t s_buffer[NVM_DATA_SIZE];
    uint32_t i, failAddr, failDat;

    __disable_irq(); // Disable all interrupts during Flash write operations

	// Prepare flash cache/prefetch/speculation
	FTFx_CACHE_ClearCachePrefetchSpeculation(&s_cacheDriver, true);

	// In case of the protected sectors at the end of the pFlash just select
	// the block from the end of pFlash to be used for operations
	// SECTOR_INDEX_FROM_END = 1 means the last sector
	// SECTOR_INDEX_FROM_END = 2 means (the last sector - 1)

	// Erase a sector starting from destAddress
	destAddress = pflashBlockBase + (pflashTotalSize - (SECTOR_INDEX_FROM_END * pflashSectorSize));
    result = FLASH_Erase(&s_flashDriver, destAddress, pflashSectorSize, kFTFx_ApiEraseKey);
    if (kStatus_FTFx_Success != result)
    {
        __enable_irq();
    	return 1;
    }

    // Prepare user buffer with data
    for (i = 0; i < NVM_DATA_SIZE; i++)
    {
        s_buffer[i] = (data >> (24-(i*8))) & 0xFF;
    }

    // Program user buffer into flash
     result = FLASH_Program(&s_flashDriver, destAddress, (uint8_t *)s_buffer, sizeof(s_buffer));
     if (kStatus_FTFx_Success != result)
     {
        __enable_irq();
     	return 1;
     }

     // Verify programming
     result = FLASH_VerifyProgram(&s_flashDriver, destAddress, sizeof(s_buffer), (const uint8_t *)s_buffer, kFTFx_MarginValueUser,
                                  &failAddr, &failDat);
     if (kStatus_FTFx_Success != result)
     {
        __enable_irq();
      	return 1;
     }

     // Clean-up
     FTFx_CACHE_ClearCachePrefetchSpeculation(&s_cacheDriver, false);

     __enable_irq();
     return 0;
}

/**************************************************************/

void KL_Flash_Read(uint32_t *data)
{
	uint32_t i;
    uint32_t s_buffer_rbc[NVM_DATA_SIZE];
    uint32_t destAddress; 	// Base address of the target memory location

	// In case of the protected sectors at the end of the pFlash just select
	// the block from the end of pFlash to be used for operations
	// SECTOR_INDEX_FROM_END = 1 means the last sector
	// SECTOR_INDEX_FROM_END = 2 means (the last sector - 1)

    // calculate address
	destAddress = pflashBlockBase + (pflashTotalSize - (SECTOR_INDEX_FROM_END * pflashSectorSize));

    for (i = 0; i < NVM_DATA_SIZE; i++)
    {
        s_buffer_rbc[i] = *(volatile uint32_t *)(destAddress + i * 4);
    }

    for (i = 0; i < NVM_DATA_SIZE; i++)
    {
        *data <<= 8;
        *data |= (s_buffer_rbc[i] & 0xFF);
    }
}

/**************************************************************/

bool KL_Check_RX(void)  // check if USB-to-Serial adapter is connected
{
	bool res;

	// re-map pin from KL_RX to GPIO E23
	gpio_pin_config_t KL_RX_config = {
	        .pinDirection = kGPIO_DigitalInput,
	        .outputLogic = 0U
	    };
	GPIO_PinInit(GPIOE, BOARD_INITPINS_KL_RX_PIN, &KL_RX_config);
    PORT_SetPinMux(BOARD_INITPINS_KL_RX_PORT, BOARD_INITPINS_KL_RX_PIN, kPORT_MuxAsGpio);
	    PORTE->PCR[BOARD_INITPINS_KL_RX_PIN] = ((PORTE->PCR[BOARD_INITPINS_KL_RX_PIN] &
	                      /* Mask bits to zero which are setting */
	                      (~(PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_ISF_MASK)))

	                     /* Pull Select: Internal pulldown resistor is enabled on the corresponding pin, if the
	                      * corresponding PE field is set. */
	                     | (uint32_t)(kPORT_PullDown));
	SysTick_DelayTicks(10);

	res = (bool)GPIO_PinRead(GPIOE, BOARD_INITPINS_KL_RX_PIN);

	// set KL_RX back to UART2_RX
    PORT_SetPinInterruptConfig(BOARD_INITPINS_KL_RX_PORT, BOARD_INITPINS_KL_RX_PIN, kPORT_InterruptOrDMADisabled);
	    PORT_SetPinMux(BOARD_INITPINS_KL_RX_PORT, BOARD_INITPINS_KL_RX_PIN, kPORT_MuxAlt4);
	    SysTick_DelayTicks(10);

	return res;
}

/**************************************************************/

void KL_Error(bool error_piezo, bool error_led)
{
	int i;

	if (error_piezo || error_led)
	{
		for (i = 0; i < 5; ++i)
		{
			if (error_led)
				LP5569_SetLED_AllOn();

			if (error_piezo)
				KL_Piezo(2600, 250, 50);
			else
				SysTick_DelayTicks(250);

			if (error_led)
				LP5569_SetLED_AllOff();

			SysTick_DelayTicks(250);
		}
	}
}

/**************************************************************/

void KL_Sleep(void)
{
	if (badge_state == ATTRACT || badge_state == COMPLETE)		// set timer for attract mode, otherwise only wake via NXH
	{
		LPTMR_StartTimer(LPTMR0_PERIPHERAL); 	// Start LPTMR for periodic interrupt (LED heartbeat)
	}

	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Disable SysTick timer interrupt while we sleep

	SMC_PreEnterStopModes();
    SMC_SetPowerModeVlps(SMC);  // Zzzz...
    SMC_PostExitStopModes();

    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk  | SysTick_CTRL_ENABLE_Msk; // Re-enable SysTick timer interrupt
	LPTMR_StopTimer(LPTMR0_PERIPHERAL); // Stop timer once we wake up
}

/**************************************************************/

// Output square wave to the piezo element using the provided parameters
// frequency (Hz), duration (ms), duty cycle (%)
void KL_Piezo(uint32_t freq_Hz, uint32_t duration_ms, uint8_t pwm_duty)
{
    // Update parameters
	if (freq_Hz > 0)
	{
		TPM_SetupPwm(TPM0_PERIPHERAL, TPM0_pwmSignalParams, 1, kTPM_EdgeAlignedPwm, freq_Hz, TPM0_CLOCK_SOURCE); // Frequency
		TPM_UpdatePwmDutycycle(TPM0_PERIPHERAL,TPM0_pwmSignalParams->chnlNumber, kTPM_EdgeAlignedPwm, pwm_duty); // Duty cycle

	    TPM_StartTimer(TPM0_PERIPHERAL, kTPM_SystemClock);  // Start the TPM counter
	}

	if (duration_ms > 0)
		SysTick_DelayTicks(duration_ms); // Duration of note (delay for specified length in ms)

    TPM_StopTimer(TPM0_PERIPHERAL);  // Stop the TPM counter
}

/**************************************************************/

void KL_Piezo_1Up(void)
{
	int i;

    for (i = 0; i < sizeof(tune_1up) / sizeof(struct note); ++i)
    {
    	KL_Piezo(tune_1up[i].freq, tune_1up[i].duration, tune_1up[i].duty);
        SysTick_DelayTicks(10);
    }
}

/**************************************************************/

void KL_Piezo_RickRoll(void)
{
	int i, led_num;

    SysTick_DelayTicks(250);
	g_random = (unsigned char)SysTick->VAL;  // Seed PRNG with current value of SysTick timer

    /*for (i = 0; i < sizeof(tune_rickroll_intro) / sizeof(struct note); ++i)
    {
    	led_num = Get_Random_Byte() % LP5569_LED_NUM;
    	if (tune_rickroll_intro[i].freq != NOTE_REST) LP5569_SetLED(led_num, LP5569_PWM[led_num]);  // Enable LED to flash along with the music
    	KL_Piezo(tune_rickroll_intro[i].freq, tune_rickroll_intro[i].duration, tune_rickroll_intro[i].duty);
    	LP5569_SetLED(led_num, 0x00);
        SysTick_DelayTicks(45);
    }
    for (i = 0; i < sizeof(tune_rickroll_verse) / sizeof(struct note); ++i)
    {
    	led_num = Get_Random_Byte() % LP5569_LED_NUM;
    	if (tune_rickroll_verse[i].freq != NOTE_REST) LP5569_SetLED(led_num, LP5569_PWM[led_num]);  // Enable LED to flash along with the music
    	PRINTF("%s", tune_rickroll_verse_lyrics[i]);
    	KL_Piezo(tune_rickroll_verse[i].freq, tune_rickroll_verse[i].duration, tune_rickroll_verse[i].duty);
    	LP5569_SetLED(led_num, 0x00);
        SysTick_DelayTicks(30);
    }*/
    for (i = 0; i < sizeof(tune_rickroll_chorus) / sizeof(struct note); ++i)
    {
    	led_num = Get_Random_Byte() % LP5569_LED_NUM;
    	if (tune_rickroll_chorus[i].freq != NOTE_REST) LP5569_SetLED(led_num, LP5569_PWM);  // Enable LED to flash along with the music
    	PRINTF("%s", tune_rickroll_chorus_lyrics[i]);
    	KL_Piezo(tune_rickroll_chorus[i].freq, tune_rickroll_chorus[i].duration, tune_rickroll_chorus[i].duty);
    	LP5569_SetLED(led_num, 0x00);
        SysTick_DelayTicks(30);
    }
}

/**************************************************************/

int KL_Setup_NXH2261(void) // Configure NXH2261 NFMI Radio
{
	// Program the NXH binary via I2C
	if (KL_Program_NXH2261(NxH2281Eep_size, NxH2281Eep))
	{
		return 1; // Fail!
	}

	LP5569_SetLED(2, 0); // Update start-up progress via LEDs
	LP5569_SetLED(3, 0);

    PRINTF("-> Executing Program...");
    KL_Reset_NXH2261();
	SysTick_DelayTicks(200); // maximum delay of NXH2261 low-power state
    PRINTF("Done!\n\r");

	// Calibration process
    PRINTF("-> Calibrating...");
    CLOCK_SetClkOutClock(SIM_CLKOUT_SEL_OSCERCLK_CLK); // Set OSCERCLK to CLKOUT
    SysTick_DelayTicks(100);

	// Toggle NXH_UPDATE to switch NXH2261 into UART Open State (we need to be here in order to calibrate)
	GPIO_PinWrite(BOARD_INITPINS_NXH_UPDATE_GPIO, BOARD_INITPINS_NXH_UPDATE_GPIO_PIN, HIGH);
    SysTick_DelayTicks(500);
	GPIO_PinWrite(BOARD_INITPINS_NXH_UPDATE_GPIO, BOARD_INITPINS_NXH_UPDATE_GPIO_PIN, LOW);

    SysTick_DelayTicks(100);

    // Toggle NXH_CAL to tell NXH2261 that we're ready to start calibration
	GPIO_PinWrite(BOARD_INITPINS_NXH_CAL_GPIO, BOARD_INITPINS_NXH_CAL_GPIO_PIN, HIGH);
    SysTick_DelayTicks(500);
	GPIO_PinWrite(BOARD_INITPINS_NXH_CAL_GPIO, BOARD_INITPINS_NXH_CAL_GPIO_PIN, LOW);

    SysTick_DelayTicks(500); // wait for calibration to complete (typical 50-150ms)
    CLOCK_SetClkOutClock(0); // Turn off CLKOUT

    PRINTF("Done!\n\r");

	LP5569_SetLED(1, 0); // Update start-up progress via LEDs
	LP5569_SetLED(4, 0);

	return 0; // Success!
}

/**************************************************************/

int KL_Program_NXH2261(const uint32_t NxH2281Eep_size, const unsigned char *NxH2281Eep)
{
	uint8_t i = 0;
	uint32_t res;
	uint8_t txbuf[12], rxbuf[12];

    PRINTF("-> Entering Bootloader...");
	KL_Reset_NXH2261();

	// After reset, the NXH2261 waits 30ms before loading code from its internal EEPROM.
	// Within this time, the host controller can send a Prevent Boot command, which forces
	// the device into bootloader mode and allows us to send new firmware to its EEPROM.
	do
	{
		res = 1;
		txbuf[0] = LOW_BYTE(NXH2261_CMD_PREVENT_BOOT);
		txbuf[1] = HIGH_BYTE(NXH2261_CMD_PREVENT_BOOT);
		txbuf[2] = 0; // tag
		I2C_WriteBulk(I2C0_PERIPHERAL, I2C_NXH2261_ADDR, txbuf, 3);
		if (!I2C_ReadBulk(I2C0_PERIPHERAL, I2C_NXH2261_ADDR, rxbuf, 4))
		{
			// if I2C was successful, response should be all 0x00
			res = rxbuf[0] | rxbuf[1] | rxbuf[2] | rxbuf[3];
		}
        SysTick_DelayTicks(5);
        ++i;
	} while (res != 0 && i < 100);

	if (i >= 100)  // Timeout
	{
		PRINTF("Error!\n\r");
		return 1;
	}

	PRINTF("Done!\n\r");

    // Get version information
	txbuf[0] = LOW_BYTE(NXH2261_CMD_GET_VERSION);
	txbuf[1] = HIGH_BYTE(NXH2261_CMD_GET_VERSION);
	txbuf[2] = 0; // tag
	I2C_WriteBulk(I2C0_PERIPHERAL, I2C_NXH2261_ADDR, txbuf, 3);
	I2C_ReadBulk(I2C0_PERIPHERAL, I2C_NXH2261_ADDR, rxbuf, 9);
	PRINTF("-> Firmware Version: 0x%02X 0x%02X\n\r", rxbuf[0], rxbuf[1]);
	PRINTF("-> Hardware Version: 0x%02X%02X 0x%02X%02X\n\r", rxbuf[3], rxbuf[2], rxbuf[6], rxbuf[5]);
	PRINTF("-> ROM Version: 0x%02X 0x%02X\n\r", rxbuf[7], rxbuf[8]);

    PRINTF("-> Programming...");

    // Enable the EEPROM in preparation to program
	txbuf[0] = LOW_BYTE(NXH2261_CMD_EEPROM_ENABLE);
	txbuf[1] = HIGH_BYTE(NXH2261_CMD_EEPROM_ENABLE);
	txbuf[2] = 0; // tag
	I2C_WriteBulk(I2C0_PERIPHERAL, I2C_NXH2261_ADDR, txbuf, 3);
	if (!I2C_ReadBulk(I2C0_PERIPHERAL, I2C_NXH2261_ADDR, rxbuf, 4))
	{
		// if I2C was successful, response should be all 0x00
		res = rxbuf[0] | rxbuf[1] | rxbuf[2] | rxbuf[3];
	}
	if (res != 0)
	{
    	PRINTF("Error!\n\r");
		return 1;
	}

	// Program the Cortex image at the primary boot location
	// EEPROM has a write endurance of 100k cycles
	if (KL_LoadImage_NXH2261(NxH2281Eep_size, NxH2281Eep, 0x0000UL))
	{
	    PRINTF("Error!\n\r");
		return 1;
	}

	// Disable the EEPROM when programming is complete
	txbuf[0] = LOW_BYTE(NXH2261_CMD_EEPROM_DISABLE);
	txbuf[1] = HIGH_BYTE(NXH2261_CMD_EEPROM_DISABLE);
	txbuf[2] = 0; // tag
	I2C_WriteBulk(I2C0_PERIPHERAL, I2C_NXH2261_ADDR, txbuf, 3);
	if (!I2C_ReadBulk(I2C0_PERIPHERAL, I2C_NXH2261_ADDR, rxbuf, 4))
	{
		// if I2C was successful, response should be all 0x00
		res = rxbuf[0] | rxbuf[1] | rxbuf[2] | rxbuf[3];
	}
	if (res != 0)
	{
	   	PRINTF("Error!\n\r");
		return 1;
	}

	PRINTF("Done!\n\r");

	return 0; // Success!
}

/**************************************************************/

int KL_LoadImage_NXH2261(uint32_t size, const uint8_t *data, uint32_t address)
{
    uint32_t i;
    uint16_t txaddr, chunkSize, cnt;
	uint32_t res = 0;
	uint8_t txbuf[NXH2261_MAX_CHUNK_SIZE + 8], rxbuf[NXH2261_MAX_CHUNK_SIZE + 8];

    i = 0;
    while (i < size)
    {
    	// Setup parameters
    	chunkSize = ((size - i) > NXH2261_MAX_CHUNK_SIZE ? NXH2261_MAX_CHUNK_SIZE : (size - i)); // number of bytes to write
    	txaddr = address + (i / 4UL); // offset (word address) into the EEPROM

    	// Unlock EEPROM memory region for a single write
    	txbuf[0] = LOW_BYTE(NXH2261_CMD_EEPROM_UNLOCK);
    	txbuf[1] = HIGH_BYTE(NXH2261_CMD_EEPROM_UNLOCK);
    	txbuf[2] = 0; // tag
    	txbuf[3] = LOW_BYTE(txaddr);
    	txbuf[4] = HIGH_BYTE(txaddr);
    	txbuf[5] = LOW_BYTE(chunkSize);
    	txbuf[6] = HIGH_BYTE(chunkSize);
    	I2C_WriteBulk(I2C0_PERIPHERAL, I2C_NXH2261_ADDR, txbuf, 7);
    	if (!I2C_ReadBulk(I2C0_PERIPHERAL, I2C_NXH2261_ADDR, rxbuf, 4))
    	{
    		// if I2C was successful, response should be all 0x00
    		res = rxbuf[0] | rxbuf[1] | rxbuf[2] | rxbuf[3];
    	}
    	if (res != 0)
    	{
    		return 1;
    	}

    	// Write data to EEPROM
    	txbuf[0] = LOW_BYTE(NXH2261_CMD_EEPROM_WRITE);
    	txbuf[1] = HIGH_BYTE(NXH2261_CMD_EEPROM_WRITE);
    	txbuf[2] = 0; // tag
    	txbuf[3] = LOW_BYTE(txaddr);
    	txbuf[4] = HIGH_BYTE(txaddr);
    	txbuf[5] = 0;
    	txbuf[6] = 0;
    	txbuf[7] = LOW_BYTE(chunkSize);
    	txbuf[8] = HIGH_BYTE(chunkSize);
    	txbuf[9] = 0;
    	txbuf[10] = 0;
    	cnt = chunkSize; // move length into its own variable so memcpy doesn't erase it
    	memcpy(&txbuf[11], data + i, cnt);
    	I2C_WriteBulk(I2C0_PERIPHERAL, I2C_NXH2261_ADDR, txbuf, 11 + chunkSize);
    	if (!I2C_ReadBulk(I2C0_PERIPHERAL, I2C_NXH2261_ADDR, rxbuf, 4))
    	{
    		// if I2C was successful, response should be all 0x00
    		res = rxbuf[0] | rxbuf[1] | rxbuf[2] | rxbuf[3];
    	}
    	if (res != 0)
    	{
    		return 1;
    	}

      i += chunkSize;
    }

	return 0; // Success!
}

/**************************************************************/

// retrieve the most recently received data packet from the ring buffer, if it exists
int KL_GetPacket_NXH2261(struct packet_of_infamy *rxPacket)
{
	size_t i;
	uint8_t ch, dataBlob[NXH2261_DATA_PACKET_SIZE], buf[NXH2261_DATA_PACKET_SIZE >> 1];

	// increment through the buffer until we find the packet header
	do
	{
		if (nxhRxIndex == nxhTxIndex)  // we've reached the end of the buffer
			return 1;

		ch = nxhRingBuffer[nxhTxIndex];
		nxhTxIndex++;
		nxhTxIndex %= LPUART0_RING_BUFFER_SIZE;
	} while (ch != 'B');

	// extract the data contents from the buffer until we reach the packet footer
	i = 0;
	while ((ch = nxhRingBuffer[nxhTxIndex]) != 'E')
	{
		if (nxhRxIndex == nxhTxIndex)  // we've reached the end of the buffer
			return 1;

		dataBlob[i] = ch;
		i++;
		nxhTxIndex++;
		nxhTxIndex %= LPUART0_RING_BUFFER_SIZE;
	}

	// remove 0xD0 padding from each nibble
	for (i = 0; i < (NXH2261_DATA_PACKET_SIZE >> 1) - 1; i++)
	{
		buf[i] = (dataBlob[i*2] & 0x0F) << 4;
		buf[i] |= dataBlob[i*2 + 1] & 0x0F;
	}

	// Place data into proper structure
	rxPacket->uid = (uint32_t)((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]); 	// unique ID
	rxPacket->type = buf[4];	// badge type
	rxPacket->magic = buf[5];	// magic token (1 = enabled)
	rxPacket->flags = buf[6];   // game flags (packed, MSB unused)
	rxPacket->unused = buf[7];	// unused

	return 0;
}

/**************************************************************/

// update NXH2261 with data packet to transmit
int KL_UpdatePacket_NXH2261(struct packet_of_infamy txPacket)
{
	size_t i = 0;
	uint8_t ch, dataBlob[NXH2261_DATA_PACKET_SIZE];

	dataBlob[0] = 'B';  // header
	dataBlob[NXH2261_DATA_PACKET_SIZE - 1] = 'E';  // footer

	const unsigned char *buf = (unsigned char*)&txPacket;

	// pad each nibble of buf with 0xD0 per the custom NXH UART protocol
	for (i = 0; i < ((NXH2261_DATA_PACKET_SIZE - 2) >> 1); i++)
	{
		dataBlob[i*2 + 1] = (0xD0 + ((buf[i] & 0xF0) >> 4));
		dataBlob[i*2 + 2] = (0xD0 + ((buf[i] & 0x0F)));
	}

	// flip ordering due to endianness
	uint8_t index[] = {0, 7, 8, 5, 6, 3, 4, 1, 2, 9, 10, 11, 12, 13, 14, 15, 16, 17};
	Reorder_Array(dataBlob, index, NXH2261_DATA_PACKET_SIZE);

	// display hex data from data packet struct (without header and footer)
	PRINTF("[*] Loading Data: 0x");
	for (i = 1; i < NXH2261_DATA_PACKET_SIZE - 1; ++i)
	{
		PRINTF("%02X", dataBlob[i]);
	}
	PRINTF("\n\r");

	// Toggle NXH_UPDATE to tell NXH2261 that we want to update data being sent
	GPIO_PinWrite(BOARD_INITPINS_NXH_UPDATE_GPIO, BOARD_INITPINS_NXH_UPDATE_GPIO_PIN, HIGH);
	SysTick_DelayTicks(100);
	GPIO_PinWrite(BOARD_INITPINS_NXH_UPDATE_GPIO, BOARD_INITPINS_NXH_UPDATE_GPIO_PIN, LOW);
	SysTick_DelayTicks(100);

	// Wait until we receive "RO" from NXH to indicate that it is ready to receive new data
	do
	{
		if (nxhRxIndex == nxhTxIndex)  // we've reached the end of the buffer
			return 1;

		ch = nxhRingBuffer[nxhTxIndex];
		nxhTxIndex++;
		nxhTxIndex %= LPUART0_RING_BUFFER_SIZE;
	} while (ch != 'R');

	// now check if 'O' is next door
	if (nxhRxIndex == nxhTxIndex)  // we've reached the end of the buffer
		return 1;

	ch = nxhRingBuffer[nxhTxIndex];
	nxhTxIndex++;
	nxhTxIndex %= LPUART0_RING_BUFFER_SIZE;
	if (ch == 'O')
	{
		// send our updated data packet to the NXH
		LPUART_WriteBlocking(LPUART0_PERIPHERAL, dataBlob, sizeof(dataBlob));
	}
	else
		return 1;

	return 0;
}

/**************************************************************/

void KL_Reset_NXH2261(void)
{
	GPIO_PinWrite(BOARD_INITPINS_NXH_nRESET_GPIO, BOARD_INITPINS_NXH_nRESET_GPIO_PIN, LOW);  // Disable NXH2261
	SysTick_DelayTicks(250);

	GPIO_PinWrite(BOARD_INITPINS_NXH_nRESET_GPIO, BOARD_INITPINS_NXH_nRESET_GPIO_PIN, HIGH); // Enable NXH2261
	SysTick_DelayTicks(10);
}

/**************************************************************/

int KL_Setup_LP5569(void) // Configure LP5569 LED Driver
{
	uint8_t i, err = 0;

	GPIO_PinWrite(BOARD_INITPINS_LED_EN_GPIO, BOARD_INITPINS_LED_EN_GPIO_PIN, LOW);
	SysTick_DelayTicks(10);

	GPIO_PinWrite(BOARD_INITPINS_LED_EN_GPIO, BOARD_INITPINS_LED_EN_GPIO_PIN, HIGH); // Enable LP5569
	SysTick_DelayTicks(10);

	// Load default LED settings
	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_CONFIG, 0x00); // Disable LP5569 while setting MISC register
	SysTick_DelayTicks(1); // Wait for register to be updated

	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_MISC, 0x39); 		// Auto-increment disabled, auto-power save, auto-charge pump, internal 32.768kHz oscillator
	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_CONFIG, 0x40); 	// Turn on LP5569
	SysTick_DelayTicks(1); // Wait for register to be updated

	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_LED_ENGINE_CONTROL2, 0xFA); // Halt all engines, enable direct LED control

	for (i = 0; i < LP5569_LED_NUM; i++)  // Control Register
	{
		err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_LED0_CONTROL + i, LP5569_Control);
	}

	for (i = 0; i < LP5569_LED_NUM; i++)  // Current Control
	{
		err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_LED0_CURRENT + i, LP5569_Current);
	}

	for (i = 0; i < LP5569_LED_NUM; i++)  // PWM Duty Cycle
	{
		err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_LED0_PWM + i, 0x00); // All LEDs off
	}

	// Disable unused LED channels
	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_LED6_CONTROL, 0x00);
	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_LED7_CONTROL, 0x00);
	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_LED8_CONTROL, 0x00);

	// Map LED to register control (not execution engine)
	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_ENGINE1_MAPPING1, 0x00); // LED8
	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_ENGINE1_MAPPING2, 0x00); // LED7..0
	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_ENGINE2_MAPPING1, 0x00); // LED8
	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_ENGINE2_MAPPING2, 0x00); // LED7..0
	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_ENGINE3_MAPPING1, 0x00); // LED8
	err += I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_ENGINE3_MAPPING2, 0x00); // LED7..0

	return err;
}

/**************************************************************/

void LP5569_SetLED(unsigned char led_num, unsigned char led_pwm)
{
	int i;

	//PRINTF("[*] Setting LED %d @ PWM %d\n\r", led_num, led_pwm);

	if (I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_LED0_PWM + led_num, led_pwm))
	{
		// if write fails, try to release the bus
		PRINTF("[*] I2C Bus Clear...");
		I2C_ReleaseBus();
		SysTick_DelayTicks(100);

		// re-initialize I2C
		I2C_MasterDeinit(I2C0_PERIPHERAL);
		SysTick_DelayTicks(10);
		I2C_MasterInit(I2C0_PERIPHERAL, &I2C0_config, I2C0_CLK_FREQ);
		SysTick_DelayTicks(10);

		// and re-attempt the transaction
		if (I2C_WriteRegister(I2C0_PERIPHERAL, I2C_LP5569_ADDR, LP5569_REG_LED0_PWM + led_num, led_pwm))
		{
			PRINTF("Error!\n\r");
		}
		else
		{
			PRINTF("Done!\n\r");
		}
	}

	for (i = 0; i < 100; ++i){};  // poor man's delay (us)
}

/**************************************************************/

void LP5569_SetLED_AllOn(void)
{
	static volatile int i;

	for (i = 0; i < LP5569_LED_NUM; i++)  // each LED
	{
		LP5569_SetLED(i, LP5569_PWM); // default brightness
	}
}

/**************************************************************/

void LP5569_SetLED_AllOff(void)
{
	static volatile int i;

	for (i = 0; i < LP5569_LED_NUM; i++)  // each LED
	{
		LP5569_SetLED(i, 0); // off
	}
}

/**************************************************************/

void LP5569_SetLED_D(unsigned char pwm_val)
{
	LP5569_SetLED(0, 0);
	LP5569_SetLED(1, 0);
	LP5569_SetLED(2, 0);
	LP5569_SetLED(3, pwm_val);
	LP5569_SetLED(4, pwm_val);
	LP5569_SetLED(5, pwm_val);
}

/**************************************************************/

void LP5569_SetLED_E(unsigned char pwm_val)
{
	LP5569_SetLED(0, pwm_val);
	LP5569_SetLED(1, pwm_val);
	LP5569_SetLED(2, pwm_val);
	LP5569_SetLED(3, 0);
	LP5569_SetLED(4, pwm_val);
	LP5569_SetLED(5, 0);
}

/**************************************************************/

void LP5569_SetLED_F(unsigned char pwm_val)
{
	LP5569_SetLED(0, pwm_val);
	LP5569_SetLED(1, pwm_val);
	LP5569_SetLED(2, pwm_val);
	LP5569_SetLED(3, pwm_val);
	LP5569_SetLED(4, 0);
	LP5569_SetLED(5, 0);
}

/**************************************************************/

void LP5569_SetLED_C(unsigned char pwm_val)
{
	LP5569_SetLED(0, pwm_val);
	LP5569_SetLED(1, pwm_val);
	LP5569_SetLED(2, pwm_val);
	LP5569_SetLED(3, 0);
	LP5569_SetLED(4, 0);
	LP5569_SetLED(5, 0);
}

/**************************************************************/

void LP5569_SetLED_O(unsigned char pwm_val)
{
	LP5569_SetLED(0, pwm_val);
	LP5569_SetLED(1, pwm_val);
	LP5569_SetLED(2, pwm_val);
	LP5569_SetLED(3, pwm_val);
	LP5569_SetLED(4, pwm_val);
	LP5569_SetLED(5, pwm_val);
}

/**************************************************************/

void LP5569_SetLED_N(unsigned char pwm_val)
{
	LP5569_SetLED(0, pwm_val);
	LP5569_SetLED(1, pwm_val);
	LP5569_SetLED(2, 0);
	LP5569_SetLED(3, pwm_val);
	LP5569_SetLED(4, pwm_val);
	LP5569_SetLED(5, 0);
}

/**************************************************************/

void LP5569_RampLED(badge_state_t ch)  // display heartbeat (ramp up/down) of specified LED pattern
{
	static volatile int j;

	for (j = 0; j <= LP5569_PWM; j += 4)  // ramp up to maximum defined brightness
	{
		if (g_nxhDetect)
		{
			g_nxhDetect = false;
			return; // exit the heartbeat if new badge data has just been received
		}

		switch (ch)
		{
			case D:
				LP5569_SetLED_D(j);
				break;

			case E:
				LP5569_SetLED_E(j);
				break;

			case F:
				LP5569_SetLED_F(j);
				break;

			case C:
				LP5569_SetLED_C(j);
				break;

			case O:
				LP5569_SetLED_O(j);
				break;

			case N:
				LP5569_SetLED_N(j);
				break;

			case ATTRACT:
			case COMPLETE:
			default:
				break;
		}

		SysTick_DelayTicks(LED_HEARTBEAT_FADE_DELAY);
	}

	LPTMR_SetTimerPeriod(LPTMR0_PERIPHERAL, LED_HEARTBEAT_WAIT_DELAY);	// Set time to sleep at LED maximum brightness
	KL_Sleep(); // Go to sleep until our delay period is complete
	if (g_nxhDetect)
	{
		g_nxhDetect = false;
		return; // exit the heartbeat if new badge data has just been received
	}

	for (j = LP5569_PWM; j >= 0; j -= 4)  // ramp down from maximum defined brightness
	{
		if (g_nxhDetect)
		{
			g_nxhDetect = false;
			return; // exit the heartbeat if new badge data has just been received
		}

		switch (ch)
		{
			case D:
				LP5569_SetLED_D(j);
				break;

			case E:
				LP5569_SetLED_E(j);
				break;

			case F:
				LP5569_SetLED_F(j);
				break;

			case C:
				LP5569_SetLED_C(j);
				break;

			case O:
				LP5569_SetLED_O(j);
				break;

			case N:
				LP5569_SetLED_N(j);
				break;

			case ATTRACT:
			case COMPLETE:
			default:
				break;
		}

		SysTick_DelayTicks(LED_HEARTBEAT_FADE_DELAY);
	}
}

/**************************************************************/

// Utility function to read single byte from specified register of the I2C device
bool I2C_ReadRegister(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t *rxBuff, uint32_t rxSize)
{
    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));

    masterXfer.slaveAddress = device_addr;
    masterXfer.direction = kI2C_Read;
    masterXfer.subaddress = reg_addr;
    masterXfer.subaddressSize = 1;
    masterXfer.data = rxBuff;
    masterXfer.dataSize = rxSize;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    /* direction=write : start+device_write;cmdbuff;xBuff; */
    /* direction=receive : start+device_write;cmdbuff;repeatStart+device_read;xBuff; */

    // does not return until the transfer succeeds or fails due to arbitration lost or receiving a NAK
    if (I2C_MasterTransferBlocking(base, &masterXfer) == kStatus_Success)
    	return 0;
    else
    	return 1;
}

/**************************************************************/

// Utility function to write single byte to specified register of the I2C device
bool I2C_WriteRegister(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t value)
{
    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));

    masterXfer.slaveAddress = device_addr;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = reg_addr;
    masterXfer.subaddressSize = 1;
    masterXfer.data = &value;
    masterXfer.dataSize = 1;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    /* direction=write : start+device_write;cmdbuff;xBuff; */
    /* direction=receive : start+device_write;cmdbuff;repeatStart+device_read;xBuff; */

    // does not return until the transfer succeeds or fails due to arbitration lost or receiving a NAK
    if (I2C_MasterTransferBlocking(base, &masterXfer) == kStatus_Success)
    	return 0;
    else
    	return 1;
}

/**************************************************************/

// Utility function to read multiple bytes (rxSize) from the specified I2C device
bool I2C_ReadBulk(I2C_Type *base, uint8_t device_addr, uint8_t *rxBuff, uint32_t rxSize)
{
    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));

    masterXfer.slaveAddress = device_addr;
    masterXfer.direction = kI2C_Read;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = rxBuff;
    masterXfer.dataSize = rxSize;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    /* direction=write : start+device_write;cmdbuff;xBuff; */
    /* direction=receive : start+device_write;cmdbuff;repeatStart+device_read;xBuff; */

    // does not return until the transfer succeeds or fails due to arbitration lost or receiving a NAK
    if (I2C_MasterTransferBlocking(base, &masterXfer) == kStatus_Success)
    	return 0;
    else
    	return 1;
}

/**************************************************************/

// Utility function to write multiple bytes (txSize) to the specified I2C device
bool I2C_WriteBulk(I2C_Type *base, uint8_t device_addr, uint8_t *txBuff, uint32_t txSize)
{
    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));

    masterXfer.slaveAddress = device_addr;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = txBuff;
    masterXfer.dataSize = txSize;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    /* direction=write : start+device_write;cmdbuff;xBuff; */
    /* direction=receive : start+device_write;cmdbuff;repeatStart+device_read;xBuff; */

    // does not return until the transfer succeeds or fails due to arbitration lost or receiving a NAK
    if (I2C_MasterTransferBlocking(base, &masterXfer) == kStatus_Success)
    	return 0;
    else
    	return 1;
}

/**************************************************************/

// Release/clear the bus due to an error condition of SDA stuck LOW
// From UM10204 I2C bus specification and user manual, pg. 20
// https://www.nxp.com/docs/en/user-guide/UM10204.pdf
void I2C_ReleaseBus(void)
{
    uint8_t i = 0, j;

    // set SCL pin as GPIO
 	gpio_pin_config_t SCL_config = {
 	        .pinDirection = kGPIO_DigitalOutput,
 	        .outputLogic = 1U
  	    };
  	    GPIO_PinInit(GPIOB, BOARD_INITPINS_SCL_PIN, &SCL_config);
        PORT_SetPinMux(BOARD_INITPINS_SCL_PORT, BOARD_INITPINS_SCL_PIN, kPORT_MuxAsGpio);
		SysTick_DelayTicks(10);

    // send 9 pulses on SCL
	// the slave device that is holding the bus LOW should release it sometime within these clocks
    for (i = 0; i < 9; i++)
    {
        GPIO_PinWrite(GPIOB, BOARD_INITPINS_SCL_PIN, 0U);
    	for (j = 0; j < 10; ++j){};  // poor man's delay (us)

    	GPIO_PinWrite(GPIOB, BOARD_INITPINS_SCL_PIN, 1U);
    	for (j = 0; j < 10; ++j){};  // poor man's delay (us)
    }

    // set SCL back to I2C0_SCL
    PORT_SetPinMux(BOARD_INITPINS_SCL_PORT, BOARD_INITPINS_SCL_PIN, kPORT_MuxAlt2);
    SysTick_DelayTicks(10);
}

/**************************************************************/

unsigned char Get_Random_Byte(void)  // PRNG
{
	unsigned char sum = 0;

	// This calculates parity on the selected bits (mask = 0xb4)
	if(g_random & 0x80)
		sum = 1;

	if(g_random & 0x20)
		sum ^= 1;

	if(g_random & 0x10)
		sum ^= 1;

	if(g_random & 0x04)
		sum ^= 1;

	g_random <<= 1;
	g_random |= sum;

	return(g_random);
}

/**************************************************************/

// print all 8 bits of a byte including leading zeros
// from https://forum.arduino.cc/index.php?topic=46320.0
void Print_Bits(uint8_t myByte)
{
    for (uint8_t mask = 0x80; mask; mask >>= 1)
    {
        if(mask & myByte)
            PRINTF("1");
        else
            PRINTF("0");
    }
}

/**************************************************************/

// Reorder elements in an array of N size based on index array
// from https://www.tutorialcup.com/array/reorder-array-indexes.htm
void Reorder_Array(uint8_t *array, uint8_t *index, uint8_t N)
{
	size_t i;
	uint8_t temp[N]; 	// temporary array

    // array[i] should present at index[i] index
    for (i = 0; i < N; i++)
    {
    	temp[index[i]] = array[i];
    }

    for (i = 0; i < N; i++)
    {
    	array[i] = temp[i];
    	index[i] = i;
    }
}

/**************************************************************/

void SysTick_DelayTicks(uint32_t n)
{
    g_systickCounter = n;
    while(g_systickCounter != 0U){};  // wait here until counter is done
}


/****************************************************************************
 ********************* Interrupt Handlers ***********************************
 ***************************************************************************/

void SysTick_Handler(void)
{
    if (g_systickCounter != 0U)
    {
        g_systickCounter--;
    }
}

/**************************************************************/

void LPUART0_SERIAL_RX_TX_IRQHANDLER(void)
{
	volatile uint8_t data;

	// If new data has arrived from the NXH...
	if (kLPUART_RxDataRegFullFlag & LPUART_GetStatusFlags(LPUART0_PERIPHERAL))
	{
		data = LPUART_ReadByte(LPUART0_PERIPHERAL);

		// If ring buffer isn't full, add the data
		if (((nxhRxIndex + 1) % LPUART0_RING_BUFFER_SIZE) != nxhTxIndex)
		{
			nxhRingBuffer[nxhRxIndex] = data;
			nxhRxIndex++;
			nxhRxIndex %= LPUART0_RING_BUFFER_SIZE;
		}
	}

	// If the UART buffer has overrun and can't store incoming data...
	if (kLPUART_RxOverrunFlag & LPUART_GetStatusFlags(LPUART0_PERIPHERAL))
	{
		data = LPUART_ReadByte(LPUART0_PERIPHERAL); // dummy read to clear buffer (could cause misalignment of data packet)
		LPUART_ClearStatusFlags(LPUART0_PERIPHERAL, kLPUART_RxOverrunFlag);
	}
}

/**************************************************************/

void LPTMR0_IRQHandler(void)
{
	g_lptmrFlag = true; // Set state of global variable

	LPTMR_ClearStatusFlags(LPTMR0_PERIPHERAL, kLPTMR_TimerCompareFlag);
}

/**************************************************************/

void PORTB_PORTC_PORTD_PORTE_IRQHandler(void)
{
	if (GPIO_PortGetInterruptFlags(GPIOC_GPIO))  // If interrupt was from Port C (NXH_DETECT)
	{
		g_nxhDetect = true;	// Set state of global variable

		GPIO_PortClearInterruptFlags(GPIOC_GPIO, 1U << BOARD_INITPINS_NXH_DETECT_PIN); 	// Clear external interrupt flag
	}
	else // Otherwise, interrupt must have been from Port E (KL_RX) via USB-to-Serial connection
	{
		GPIO_PortClearInterruptFlags(GPIOE, 1U << BOARD_INITPINS_KL_RX_PIN);  // Clear external interrupt flag
	}
}

/**************************************************************/

// The End!
