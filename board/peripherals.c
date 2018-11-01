/*
 * The Clear BSD License
 * Copyright 2017-2018 NXP
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
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

/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!GlobalInfo
product: Peripherals v1.0
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/*******************************************************************************
 * Included files
 ******************************************************************************/
#include "peripherals.h"
#include "fsl_ftm.h"

/*******************************************************************************
 * BOARD_InitBootPeripherals function
 * Set up E clock as PWM output from FTM0_CH0 on EX_5 pin (jumper to MCU_E)
 ******************************************************************************/

// TODO(nk): clock source is not being set to FIRCDIV1(CLKS=2); it's at CLKS=1
#define FTM_SOURCE_CLOCK (CLOCK_GetFreq(kCLOCK_ScgFircAsyncDiv1Clk))

void BOARD_InitBootPeripherals(void)
{
	ftm_config_t config;
	FTM_GetDefaultConfig(&config);
	config.prescale = kFTM_Prescale_Divide_1;
	FTM_Init(FTM0, &config);

	ftm_chnl_pwm_signal_param_t ftmParam = {
			.chnlNumber = 0,
			.dutyCyclePercent = 50,
			.firstEdgeDelayPercent = 0,
			.level = kFTM_HighTrue
	};
	FTM_SetupPwm(FTM0, &ftmParam, 1, kFTM_EdgeAlignedPwm, 894886U, FTM_SOURCE_CLOCK);	// approx. /33

    FTM_StartTimer(FTM0, kFTM_FixedClock);	// use FIRCDIV1
}
