/*
 * arm_emulator.h
 *
 *  Created on: Oct 3, 2018
 *      Author: ned
 */

// PTA8 (MCU_E) alt.5 is FTM_FLT3
// TODO(nk): would be nice to use a timer output for E clock!
// could use any of EX1..EX5 for this.

// PTE8 (EX_1) alt.2 is FTM0_CH6
// PTC3 (EX_2) alt.2 is FTM0_CH3
// PTC2 (EX_3) alt.2 is FTM0_CH2
// PTC1 (EX_4) alt.2 is FTM0_CH1
// PTC0 (EX_5) alt.2 is FTM0_CH0
// PTC17 (EX_6) default is ADC0_SE15
// PTC16 (EX_7) default is ADC0_SE14
// PTC15 (EX_8) default is ADC0_SE13

#pragma once

#include <stdint.h>

/* ----- typedefs for data and offset types ----- */

#define INLINE static inline

// aliases for clearer code

#define BOARD_DATA_GPIO BOARD_INITPINS_MCU_D0_GPIO
#define BOARD_DATA_PORT BOARD_INITPINS_MCU_D0_PORT
#define BOARD_DATA_MASK 0x00FF

#define BOARD_ADDR_GPIO BOARD_INITPINS_MCU_A0_GPIO
#define BOARD_ADDR_PORT BOARD_INITPINS_MCU_A0_PORT

#define BOARD_VMA_MASK (1U << BOARD_INITPINS_MCU_VMA_PIN)
#define BOARD_RW_MASK  (1U << BOARD_INITPINS_MCU_RW_PIN)

#define BOARD_LED_MASK  (0x000FU << BOARD_INITPINS_LED_1_PIN)
#define BOARD_LED_SHIFT (0x0001U << BOARD_INITPINS_LED_1_PIN)

#define BOARD_READ_RW_MASK BOARD_RW_MASK
#define BOARD_WRITE_RW_MASK 0U

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

	ROM_1_BASE = 0x8000,
	ROM_1_SIZE = 0x7FFF,
	ROM_2_BASE = 0x4000,
	ROM_2_SIZE = 0x3FFF,

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
	ROM_END = ROM_BASE + ROM_SIZE - 1
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
	ROM_INDEX,
	NUM_MEMORY_RANGES
};

// define set/clear for EXT1 .. EXT8 pins

#define DEFINE_EXT_FUNCTIONS(num) \
INLINE void setExtOut ## num() { BOARD_INITPINS_EX_ ## num ## _GPIO->PSOR = (1U << BOARD_INITPINS_EX_ ## num ## _PIN); } \
INLINE void clearExtOut ## num () { BOARD_INITPINS_EX_ ## num ## _GPIO->PCOR = (1U << BOARD_INITPINS_EX_ ## num ## _PIN); }

DEFINE_EXT_FUNCTIONS(8)
DEFINE_EXT_FUNCTIONS(7)
DEFINE_EXT_FUNCTIONS(6)
DEFINE_EXT_FUNCTIONS(5)
DEFINE_EXT_FUNCTIONS(4)
DEFINE_EXT_FUNCTIONS(3)
DEFINE_EXT_FUNCTIONS(2)
DEFINE_EXT_FUNCTIONS(1)

#define DEFINE_LED_FUNCTIONS(num) \
INLINE void setLED ## num() { BOARD_INITPINS_LED_ ## num ## _GPIO->PSOR = (1U << BOARD_INITPINS_LED_ ## num ## _PIN); } \
INLINE void clearLED ## num() { BOARD_INITPINS_LED_ ## num ## _GPIO->PCOR = (1U << BOARD_INITPINS_LED_ ## num ## _PIN); } \
INLINE void ledOn ## num() { clearLED ## num(); } \
INLINE void ledOff ## num() { setLED ## num(); } \

DEFINE_LED_FUNCTIONS(4)
DEFINE_LED_FUNCTIONS(3)
DEFINE_LED_FUNCTIONS(2)
DEFINE_LED_FUNCTIONS(1)

extern RomRange romRanges[];

extern MemoryRange memoryRanges[NUM_MEMORY_RANGES];

constexpr MemoryRange const* findMemoryRange(uint16_t addr) {
	if (addr > ROM_BASE) { return memoryRanges+ROM_INDEX; }
	if (addr > PIA_BASE) { return memoryRanges+PIA_INDEX; }
#if HAS_SEPARATE_RAM
	if (addr > CMOS_RAM_BASE) { return memoryRanges+CMOS_RAM_INDEX; }
#endif
	return memoryRanges+RAM_INDEX;	// assumes RAM at lowest addresses
}

INLINE uint8_t cpu_readmem_internal(MemoryRange const * range, uint16_t addr) {
	return *(uint8_t const *)(range->internalAddress + (addr - range->baseAddress));
}

INLINE uint8_t cpu_read_rom_internal(uint16_t addr) {
	static MemoryRange const &rom = memoryRanges[ROM_INDEX];
	return *(uint8_t const *)(rom.internalAddress + (addr - rom.baseAddress));
}

INLINE void cpu_writemem_internal(MemoryRange const * range, uint16_t addr, uint8_t value) {
	*(uint8_t *)(range->internalAddress + (addr - range->baseAddress)) = value;
}

static inline void delayLoop(uint32_t delay) {
	uint32_t volatile i = delay;
	while (i-- != 0) {
		;
	}
}

INLINE uint8_t cpu_readmem_external(uint16_t addr) {
	// TODO(nk): wait for E falling edge
	setExtOut8();	// DEBUG
	// drive E low
	// BOARD_INITPINS_MCU_E_GPIO->PCOR = (1U << BOARD_INITPINS_MCU_E_PIN);
	// output addr | R | VMA
	BOARD_ADDR_GPIO->PDOR = (uint32_t)addr | BOARD_READ_RW_MASK | BOARD_VMA_MASK;
	// set D0-D7 to inputs
	BOARD_DATA_GPIO->PDDR = BOARD_DATA_INPUT_DIR;
	// TODO(nk): wait for E rising edge

	// wait at least 160 nsec
	delayLoop(1);

	// drive E high (TODO: make E an input)
	BOARD_INITPINS_MCU_E_GPIO->PSOR = (1U << BOARD_INITPINS_MCU_E_PIN);

	// wait at least 450 nsec
	delayLoop(4);

	// TODO(nk): wait for E to go low
	// sample data lines
	uint8_t retval = BOARD_DATA_GPIO->PDIR & BOARD_DATA_MASK;

	// drive E low (TODO: make E an input)
	BOARD_INITPINS_MCU_E_GPIO->PCOR = (1U << BOARD_INITPINS_MCU_E_PIN);

	// 20ns hold time?
	// drive VMA low
	BOARD_ADDR_GPIO->PDOR = (uint32_t)addr | BOARD_READ_RW_MASK;

	clearExtOut8();	// DEBUG
	return retval;
}

INLINE void cpu_writemem_external(uint16_t addr, uint8_t value) {
	// drive E low
	// BOARD_INITPINS_MCU_E_GPIO->PCOR = (1U << BOARD_INITPINS_MCU_E_PIN);
	// output addr | R | VMA
	BOARD_ADDR_GPIO->PDOR = (uint32_t)addr | BOARD_WRITE_RW_MASK | BOARD_VMA_MASK;
	// set D0-D7 to outputs
	BOARD_DATA_GPIO->PDDR = BOARD_DATA_OUTPUT_DIR;
	// output data value
	BOARD_DATA_GPIO->PDOR = (BOARD_DATA_GPIO->PDIR & 0xF000);
	// wait at least 160nsec
	// drive E high
	BOARD_INITPINS_MCU_E_GPIO->PSOR = (1U << BOARD_INITPINS_MCU_E_PIN);
	// wait 500ns
	// drive E low
	BOARD_INITPINS_MCU_E_GPIO->PCOR = (1U << BOARD_INITPINS_MCU_E_PIN);
	// wait 500ns
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

// assuming that all ROM is internal...

INLINE uint8_t cpu_readop(uint16_t addr) { return cpu_read_rom_internal(addr); }

INLINE uint8_t cpu_readop_arg(uint16_t addr) { return cpu_readop(addr); }

void logerror(const char *text, ...);
