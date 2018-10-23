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

#ifndef ARM_EMULATOR_H_
#define ARM_EMULATOR_H_

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

// note that address bus is not fully decoded!
// ROM region at 0x5000-0x7FFF also appears at 0xD000-0xFFFF
#define A15_MASK 0x8000

enum {
	RAM_BASE      = 0x0000,	// IC13/IC16 2114 RAM 256 bytes (aliased at 0200-03FF, 1100-11FF)
	CMOS_RAM_BASE = 0x0100,	// IC19 5101 CMOS RAM 256 bytes

	// always external: 0x2000..0x3fff
	PIA_BASE   =  0x2000,
	PIA_5_BASE 	= 0x2100,	// IC36 (2100-2103) (sound, comma, interface) !A14|A13|A8
	PIA_4_BASE 	= 0x2200,	// (IC5 on driver board) (solenoids)
	PIA_3_BASE 	= 0x2400,	// (IC10 on driver board) (lamps)
	PIA_1_BASE 	= 0x2800,	// IC18 (2800-2803) (displays, DIP switches) !A14|A13|A11
	PIA_7_BASE  = 0x2C00,	// system 11B CPU.U41
	PIA_2_BASE 	= 0x3000,	// (IC11 on driver board) (switches)
	PIA_8_BASE  = 0x3400,   // system 11B CPU.U42
	PIA_END     = PIA_8_BASE + 3,

	ROM_BASE      = 0x5000 | A15_MASK,  // entire ROM region (12K) (0xD000-0xFFFF)
	ROM_5800_BASE = 0x5800 | A15_MASK,	// IC26 2316 game ROM 2K (D800-DFFF)
	ROM_6000_BASE = 0x6000 | A15_MASK,	// IC14 2316 game ROM 2K (or IC21/22?) (E000-E7FF)
	ROM_6800_BASE = 0x6800 | A15_MASK,	// IC20 2316 system ROM 2K (E800-EFFF)
	ROM_7000_BASE = 0x7000 | A15_MASK,	// IC17 2332 system ROM 4K (F000-FFFF)
};

typedef struct MemoryRange {
	uint16_t baseAddress;
	uint16_t endAddress;
	uint8_t *internalAddress;
	uint8_t isExternal: 1;
	uint8_t isPIA: 1;
	uint8_t isWritable: 1;
} MemoryRange;

enum {
	MR_INTERNAL = 0,
	MR_EXTERNAL,
	MR_NOT_PIA = 0,
	MR_PIA = 1,
	MR_WRITABLE = 1,
	MR_READONLY = 0
};

INLINE void setExtOut8() {
	BOARD_INITPINS_EX_8_GPIO->PSOR = (1U << BOARD_INITPINS_EX_8_PIN);
}
INLINE void clearExtOut8() {
	BOARD_INITPINS_EX_8_GPIO->PCOR = (1U << BOARD_INITPINS_EX_8_PIN);
}
INLINE void setExtOut7() {
	BOARD_INITPINS_EX_7_GPIO->PSOR = (1U << BOARD_INITPINS_EX_7_PIN);
}
INLINE void clearExtOut7() {
	BOARD_INITPINS_EX_7_GPIO->PCOR = (1U << BOARD_INITPINS_EX_7_PIN);
}

extern uint32_t led_states;	// TODO(nk): write LED on/off/toggle/pulse code

extern MemoryRange memoryRanges[];

INLINE MemoryRange* findMemoryRange(uint16_t addr) {
	if (addr > ROM_BASE) { return memoryRanges+3; }
	if (addr > PIA_BASE) { return memoryRanges+2; }
	if (addr > CMOS_RAM_BASE) { return memoryRanges+1; }
	return memoryRanges+0;
}

INLINE uint8_t cpu_readmem_internal(MemoryRange * const range, uint16_t addr) {
	return *(uint8_t const *)(range->internalAddress + addr - range->baseAddress);
}

INLINE void cpu_writemem_internal(MemoryRange * const range, uint16_t addr, uint8_t value) {
	*(uint8_t *)(range->internalAddress + addr - range->baseAddress) = value;
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
	// set D0-D7 to outputs
	BOARD_DATA_GPIO->PDDR = BOARD_DATA_OUTPUT_DIR;
	// output data value
	BOARD_DATA_GPIO->PDOR = (BOARD_DATA_GPIO->PDIR & 0xF000);
	// output addr | R | VMA
	BOARD_ADDR_GPIO->PDOR = (uint32_t)addr | BOARD_WRITE_RW_MASK | BOARD_VMA_MASK;
	// wait at least 160nsec
	// drive E high
	BOARD_INITPINS_MCU_E_GPIO->PSOR = (1U << BOARD_INITPINS_MCU_E_PIN);
	// wait 500ns
	// drive E low
	BOARD_INITPINS_MCU_E_GPIO->PCOR = (1U << BOARD_INITPINS_MCU_E_PIN);
	// wait 500ns
}


INLINE uint8_t cpu_readmem16(uint16_t addr) {
	MemoryRange* range = findMemoryRange(addr);
	if (range->internalAddress) {
		return cpu_readmem_internal(range, addr);
	} else {
		return cpu_readmem_external(addr);
	}
}

INLINE void cpu_writemem16(uint16_t addr, uint8_t val) {
	MemoryRange* range = findMemoryRange(addr);
	if (range->internalAddress) {
		cpu_writemem_internal(range, addr, val);
	} else {
		cpu_writemem_external(addr, val);
	}
}

// assuming that all ROM is external...

INLINE uint8_t cpu_readop(uint16_t addr) { return cpu_readmem_external(addr); }

INLINE uint8_t cpu_readop_arg(uint16_t addr) { return cpu_readop(addr); }

void logerror(const char *text, ...);


#endif /* ARM_EMULATOR_H_ */
