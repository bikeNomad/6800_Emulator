/*
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
 * @file    MKE18F512xxx16_Project.cpp
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKE18F16.h"
#include "fsl_debug_console.h"
#include "fsl_crc.h"
/* TODO: insert other include files here. */

/* TODO: insert other definitions and declarations here. */
#include "arm_emulator.h"

/*!
 * @brief Init for CRC-32.
 * @details Init CRC peripheral module for CRC-32 protocol.
 *          width=32 poly=0x04c11db7 init=0xffffffff refin=true refout=true xorout=0xffffffff check=0xcbf43926
 *          name="CRC-32"
 *          http://reveng.sourceforge.net/crc-catalogue/
 */
static void InitCrc32(CRC_Type *base, uint32_t seed) {
	crc_config_t config;

	config.polynomial = 0x04C11DB7U;
	config.seed = seed;
	config.reflectIn = true;
	config.reflectOut = true;
	config.complementChecksum = true;
	config.crcBits = kCrcBits32;
	config.crcResult = kCrcFinalChecksum;

	CRC_Init(base, &config);
}

uint32_t GetCrc32(uint16_t extAddressBase, uint16_t nBytes) {
	CRC_Type *base = CRC0;
	InitCrc32(base, 0xFFFFFFFFU);
	for (uint32_t address = extAddressBase; nBytes-- > 0; address++) {
		uint8_t byte = cpu_readmem_external(address);
		CRC_WriteData(base, &byte, 1);
	}
	return CRC_Get32bitResult(base);
}

void PrintCrc32(uint16_t extAddressBase, uint16_t nBytes) {
	uint32_t crc = GetCrc32(extAddressBase, nBytes);
	PRINTF("%04X-%04X CRC=%04X\r\n", extAddressBase, extAddressBase+nBytes-1, crc);
}

int main(void) {
  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();	// slow clock
    BOARD_FastClock();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();

    PRINTF("Pinball ROM CRC check\r\n");

    while (1) {
#ifdef SYSTEM_11
    	PrintCrc32(ROM_2_BASE, ROM_2_SIZE);
    	PrintCrc32(ROM_1_BASE, ROM_1_SIZE);
#else
    	PrintCrc32(ROM_5800_BASE, 0x0800);
    	PrintCrc32(ROM_6000_BASE, 0x0800);
    	PrintCrc32(ROM_6800_BASE, 0x0800);
    	PrintCrc32(ROM_7000_BASE, 0x1000);
#endif
    }
}

