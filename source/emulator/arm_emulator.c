/*
 * arm_emulator.c
 *
 *  Created on: Oct 3, 2018
 *      Author: ned
 */
#include "fsl_common.h"
#include "pin_mux.h"
#include "arm_emulator.h"

uint32_t led_states;

static uint8_t RAM[256];

MemoryRange memoryRanges[] = {
	// base             end					intAddr ext pia writable
	{ RAM_BASE,      	RAM_BASE+255, 		RAM,	0, 	0, 	1 },	// RAM
	{ CMOS_RAM_BASE, 	CMOS_RAM_BASE+255, 	NULL,	1, 	0, 	1 },	// CMOS RAM (persistent, external)
	{ PIA_BASE, 		PIA_END,  			NULL,	1, 	1, 	1 },	// PIA
	{ ROM_BASE,			ROM_BASE+0x2FFF,	NULL,	1,	0,	0 },	// all ROM
};
