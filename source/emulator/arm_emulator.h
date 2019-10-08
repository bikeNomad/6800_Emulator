/*
 * arm_emulator.h
 *
 *  Created on: Oct 3, 2018
 *      Author: ned
 */

#pragma once

#include <stdint.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"

/* ----- typedefs for data and offset types ----- */

#define INLINE static inline

// aliases for clearer code

#define BOARD_DATA_GPIO BOARD_INITPINS_MCU_D0_GPIO
#define BOARD_DATA_PORT BOARD_INITPINS_MCU_D0_PORT
#define BOARD_DATA_MASK 0x00FF

#define BOARD_ADDR_GPIO BOARD_INITPINS_MCU_A0_GPIO
#define BOARD_ADDR_PORT BOARD_INITPINS_MCU_A0_PORT
#define BOARD_ADDR_MASK 0xFFFF

// PORTA: MCU_NMI, MCU_RESET, MCU_IRQ, MCU_E
#define BOARD_IRQ_GPIO BOARD_INITPINS_MCU_IRQ_GPIO
#define BOARD_IRQ_PORT BOARD_INITPINS_MCU_IRQ_PORT

#define BOARD_E_GPIO BOARD_INITPINS_MCU_E_GPIO
#define BOARD_E_PORT BOARD_INITPINS_MCU_E_PORT

// PORTA pins
#define BOARD_IRQ_MASK (1U << BOARD_INITPINS_MCU_IRQ_PIN)	/* input active low */
#define BOARD_NMI_MASK (1U << BOARD_INITPINS_MCU_NMI_PIN)	/* input active low */
#define BOARD_VMA_MASK (1U << BOARD_INITPINS_MCU_VMA_PIN)	/* output */
#define BOARD_RW_MASK  (1U << BOARD_INITPINS_MCU_RW_PIN)	/* output: low=write */
#define BOARD_E_MASK 	(1U << BOARD_INITPINS_MCU_E_PIN)	/* input; should jumper to EX_5 */
#define BOARD_RESET_MASK (1U << BOARD_INITPINS_MCU_RESET_PIN)	/* input: low=reset */

#define BOARD_READ_RW_MASK BOARD_RW_MASK
#define BOARD_WRITE_RW_MASK 0U

// PORTB pins
#define BOARD_LED_MASK  (0x000FU << BOARD_INITPINS_LED_1_PIN)
#define BOARD_LED_SHIFT (0x0001U << BOARD_INITPINS_LED_1_PIN)

// PDDR bits=1: output
// remember high 4 bits are LED outputs
#define BOARD_DATA_LED_MASK 0xF000

#define BOARD_DATA_INPUT_DIR  (0x00 | BOARD_DATA_LED_MASK)
#define BOARD_DATA_OUTPUT_DIR (0xFF | BOARD_DATA_LED_MASK)

// note that address bus in System3 games is not fully decoded!
// ROM region at 0x5000-0x7FFF also appears at 0xD000-0xFFFF
#define A15_MASK 0x8000

#ifdef SYSTEM_11
#define HAS_SEPARATE_RAM 0
#else
#define HAS_SEPARATE_RAM 1
#endif

enum {
	// always external: 0x2000..0x3fff
	PIA_BASE = 0x2000,
	PIA_SIZE = 0x1000,
#ifdef SYSTEM_11
	RAM_BASE = 0x0000,
	RAM_SIZE = 0x0800,
	ROM_BASE = 0x4000,
	ROM_SIZE = 0xC000,	// 48K ROM

	ROM_2_BASE = 0x4000,
	ROM_2_SIZE = 0x4000,
	ROM_1_BASE = 0x8000,
	ROM_1_SIZE = 0x8000,

#else // SYSTEM 3 (BK)
	RAM_BASE = 0x0000,// IC13/IC16 2114 RAM 256 bytes (aliased at 0200-03FF, 1100-11FF)
	RAM_SIZE = 0x0100,
	CMOS_RAM_BASE = 0x0100,	// IC19 5101 CMOS RAM 256 bytes
	CMOS_RAM_SIZE = 0x0100,

	ROM_SIZE = 0x3000,
	ROM_BASE = 0x5000 | A15_MASK,  // entire ROM region (12K) (0xD000-0xFFFF)
	ROM_5800_BASE = 0x5800 | A15_MASK,	// IC26 2316 game ROM 2K (D800-DFFF)
	ROM_6000_BASE = 0x6000 | A15_MASK,// IC14 2316 game ROM 2K (or IC21/22?) (E000-E7FF)
	ROM_6800_BASE = 0x6800 | A15_MASK,	// IC20 2316 system ROM 2K (E800-EFFF)
	ROM_7000_BASE = 0x7000 | A15_MASK,	// IC17 2332 system ROM 4K (F000-FFFF)
#endif
	RAM_END = RAM_BASE + RAM_SIZE - 1,
	PIA_END = PIA_BASE + PIA_SIZE -1,
	ROM_1_END = ROM_1_BASE + ROM_1_SIZE - 1,
	ROM_2_END = ROM_2_BASE + ROM_2_SIZE - 1,
};


typedef struct MemoryRange {
	uint16_t baseAddress;
	uint16_t endAddress;
	uint8_t *internalAddress;
	uint8_t isExternal: 1;
	uint8_t isPIA: 1;
	uint8_t isWritable: 1;
} MemoryRange;

typedef struct RomRange {
	uint16_t baseAddress;
	uint16_t size;
	uint32_t expectedCrc;
	uint32_t actualCrc;
} RomRange;

enum {
	MR_INTERNAL = 0,
	MR_EXTERNAL,
	MR_NOT_PIA = 0,
	MR_PIA = 1,
	MR_WRITABLE = 1,
	MR_READONLY = 0
};

enum MemoryRangeIndex {
	RAM_INDEX = 0,
#if HAS_SEPARATE_RAM
	CMOS_RAM_INDEX,
#endif
	PIA_INDEX,
	ROM_2_INDEX,
	ROM_1_INDEX,
	NUM_MEMORY_RANGES
};

// define set/clear for EXT1 .. EXT8 pins

#define DEFINE_EXT_FUNCTIONS(num) \
INLINE void setExtOut ## num() { BOARD_INITPINS_EX_ ## num ## _GPIO->PSOR = (1U << BOARD_INITPINS_EX_ ## num ## _PIN); } \
INLINE void clearExtOut ## num () { BOARD_INITPINS_EX_ ## num ## _GPIO->PCOR = (1U << BOARD_INITPINS_EX_ ## num ## _PIN); }

DEFINE_EXT_FUNCTIONS(8)
DEFINE_EXT_FUNCTIONS(7)
DEFINE_EXT_FUNCTIONS(6)
// EXT OUT 5 is connected to MCU E clock pin
DEFINE_EXT_FUNCTIONS(4)
DEFINE_EXT_FUNCTIONS(3)
DEFINE_EXT_FUNCTIONS(2)
DEFINE_EXT_FUNCTIONS(1)

#define DEFINE_LED_FUNCTIONS(num) \
INLINE void setLED ## num() { BOARD_INITPINS_LED_ ## num ## _GPIO->PSOR = (1U << BOARD_INITPINS_LED_ ## num ## _PIN); } \
INLINE void clearLED ## num() { BOARD_INITPINS_LED_ ## num ## _GPIO->PCOR = (1U << BOARD_INITPINS_LED_ ## num ## _PIN); } \
INLINE void ledOn ## num() { clearLED ## num(); } \
INLINE void ledOff ## num() { setLED ## num(); } \

// TODO(nk): LEDs have become SPI pins for debugging.
// LED_1 (PTB14): SCK
// LED_3 (PTB16): MOSI
// LED_4 (PTB17): CS3
// DEFINE_LED_FUNCTIONS(4)
// DEFINE_LED_FUNCTIONS(3)
DEFINE_LED_FUNCTIONS(2)
// DEFINE_LED_FUNCTIONS(1)

extern RomRange romRanges[];

extern MemoryRange memoryRanges[NUM_MEMORY_RANGES];

static_assert(ROM_1_BASE > ROM_2_BASE, "roms out of order");
static_assert(ROM_2_BASE > PIA_BASE, "PIA address out of order");

#if __cplusplus
constexpr
#else
INLINE
#endif
MemoryRange const* findMemoryRange(uint16_t addr) {
	if (addr >= ROM_1_BASE) { return memoryRanges+ROM_1_INDEX; }
	if (addr >= ROM_2_BASE) { return memoryRanges+ROM_2_INDEX; }
	if (addr >= PIA_BASE) { return memoryRanges+PIA_INDEX; }
#if HAS_SEPARATE_RAM
	if (addr >= CMOS_RAM_BASE) { return memoryRanges+CMOS_RAM_INDEX; }
#endif
	return memoryRanges+RAM_INDEX;	// assumes RAM at lowest addresses
}

INLINE uint8_t cpu_readmem_internal(MemoryRange const * range, uint16_t addr) {
	return *(uint8_t const *)(range->internalAddress + (addr - range->baseAddress));
}

INLINE uint8_t cpu_read_rom_internal(uint16_t addr) {
	if (addr >= ROM_1_BASE) {
		static MemoryRange const *rom1 = &memoryRanges[ROM_1_INDEX];
		return *(uint8_t const *)(rom1->internalAddress + (addr - rom1->baseAddress));
	}
	/* if (addr > ROM_2_BASE) */ else {
		static MemoryRange const *rom2 = &memoryRanges[ROM_2_INDEX];
		return *(uint8_t const *)(rom2->internalAddress + (addr - rom2->baseAddress));
	}
}

INLINE void cpu_writemem_internal(MemoryRange const * range, uint16_t addr, uint8_t value) {
	*(uint8_t *)(range->internalAddress + (addr - range->baseAddress)) = value;
}

static inline void waitForElow() {
	while (BOARD_E_GPIO->PDIR & BOARD_E_MASK) { /* spin until E low */ }
}

static inline void waitForEhigh() {
	while (!(BOARD_E_GPIO->PDIR & BOARD_E_MASK)) { /* spin until E high */ }
}

// FTM0->CNT: 0x00..0x20
// each count is 1.1usec/33 = 33nsec
// read data setup time >= 100ns
// E goes low when CNT == 0x10
static inline void waitForEalmostLow() {
	while (FTM0->CNT < (0x10 - 3)) { /* spin */ }
}


INLINE uint8_t cpu_readmem_external(uint16_t addr) {
	setExtOut8();	// DEBUG

	// sync with leading edge of E:
	waitForEhigh();
	waitForElow();

	// output addr | R | VMA
	BOARD_ADDR_GPIO->PDOR = (uint32_t)addr | BOARD_READ_RW_MASK | BOARD_VMA_MASK;
	// set D0-D7 to inputs
	BOARD_DATA_GPIO->PDDR = BOARD_DATA_INPUT_DIR;

	waitForEhigh();	// FTM0->CNT just overflowed to 0

	waitForEalmostLow();

	// sample data lines
	uint8_t retval = BOARD_DATA_GPIO->PDIR & BOARD_DATA_MASK;

	waitForElow();

	// 20ns hold time?
	// drive VMA low
	BOARD_ADDR_GPIO->PDOR = (uint32_t)addr | BOARD_READ_RW_MASK;

	clearExtOut8();	// DEBUG
	return retval;
}


INLINE void cpu_writemem_external(uint16_t addr, uint8_t value) {
	setExtOut8();	// DEBUG

	// sync with leading edge of E:
	waitForEhigh();
	waitForElow();

	// output addr | R | VMA
	BOARD_ADDR_GPIO->PDOR = (uint32_t)addr | BOARD_WRITE_RW_MASK | BOARD_VMA_MASK;
	// set D0-D7 to outputs
	BOARD_DATA_GPIO->PDDR = BOARD_DATA_OUTPUT_DIR;
	// output data value
	BOARD_DATA_GPIO->PDOR = (BOARD_DATA_GPIO->PDIR & 0xF000) | value;

	waitForEhigh();	// FTM0->CNT just overflowed to 0

	// TODO(nk): wait for almost low? check setup times...
	waitForElow();

	// 20ns hold time?
	// drive VMA low
	BOARD_ADDR_GPIO->PDOR = (uint32_t)addr | BOARD_WRITE_RW_MASK;

	clearExtOut8();	// DEBUG
}


INLINE uint8_t cpu_readmem16(uint16_t addr) {
	MemoryRange const * range = findMemoryRange(addr);
	if (range->internalAddress) {
		return cpu_readmem_internal(range, addr);
	} else {
		return cpu_readmem_external(addr);
	}
}


INLINE void cpu_writemem16(uint16_t addr, uint8_t val) {
	MemoryRange const * range = findMemoryRange(addr);
	if (range->internalAddress) {
		cpu_writemem_internal(range, addr, val);
	} else {
		cpu_writemem_external(addr, val);
	}
}


static void waitEcycles(uint32_t cycles) {
	while (cycles-- > 0) {
		waitForEhigh();
		waitForEalmostLow();
	}
}


// assuming that all ROM is internal...

INLINE uint8_t cpu_readop(uint16_t addr) {
	DEBUG_ADDRESS_OUT(&addr);
	return cpu_read_rom_internal(addr);
}

INLINE uint8_t cpu_readop_arg(uint16_t addr) { return cpu_read_rom_internal(addr); }

#define logerror(...) PRINTF(__VA_ARGS__)

void copyExternalToInternal(uint16_t extStart, uint16_t size);

void copyRomsToRam();

