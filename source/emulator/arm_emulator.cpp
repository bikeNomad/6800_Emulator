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

static uint8_t RAM[ RAM_SIZE ];
static uint8_t ROM[ ROM_SIZE ];

MemoryRange memoryRanges[] = {
	// base             end					intAddr ext pia writable
	{ RAM_BASE,      	RAM_END, 			RAM,	0, 	0, 	1 },	// RAM
#if HAS_SEPARATE_RAM
	{ CMOS_RAM_BASE, 	CMOS_RAM_END, 		NULL,	1, 	0, 	1 },	// CMOS RAM (persistent, external)
#endif
	{ PIA_BASE, 		PIA_END,  			NULL,	1, 	1, 	1 },	// PIA
	{ ROM_BASE,			ROM_END,			ROM,	0,	0,	0 },	// all ROM
};
