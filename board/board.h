/*
 * The Clear BSD License
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
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

#ifndef _BOARD_H_
#define _BOARD_H_

#include "clock_config.h"
#include "fsl_gpio.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief The board name */
#define BOARD_NAME "TWR-KE18F"

/*! @brief The UART to use for debug messages. */
#define BOARD_USE_UART
#define BOARD_DEBUG_UART_TYPE DEBUG_CONSOLE_DEVICE_TYPE_LPUART
#define BOARD_DEBUG_UART_BASEADDR (uint32_t) LPUART1
#define BOARD_DEBUG_UART_INSTANCE 1U
#define BOARD_DEBUG_UART_CLKSRC kCLOCK_ScgSysPllAsyncDiv2Clk
#define BOARD_DEBUG_UART_CLK_FREQ CLOCK_GetIpFreq(kCLOCK_Lpuart1)

#define BOARD_ACCEL_I2C_BASEADDR LPI2C0
#define BOARD_ACCEL_I2C_CLOCK_FREQ CLOCK_GetIpFreq(kCLOCK_Lpi2c0)

#ifndef BOARD_DEBUG_UART_BAUDRATE
#define BOARD_DEBUG_UART_BAUDRATE 115200
#endif /* BOARD_DEBUG_UART_BAUDRATE */

/*! @brief The lpi2c instance used for lpi2c connection by default */
#define BOARD_I2C_BASEADDR LPI2C0

/*! @brief The rtc instance used for board. */
#define BOARD_RTC_FUNC_BASEADDR RTC

/*! @brief Define the port interrupt number for the board switches */
#ifndef BOARD_SW2_GPIO
#define BOARD_SW2_GPIO GPIOD
#endif
#ifndef BOARD_SW2_PORT
#define BOARD_SW2_PORT PORTD
#endif
#ifndef BOARD_SW2_GPIO_PIN
#define BOARD_SW2_GPIO_PIN 3U
#endif
#define BOARD_SW2_IRQ PORTD_IRQn
#define BOARD_SW2_IRQ_HANDLER PORTD_IRQHandler
#define BOARD_SW2_NAME "SW2"

#ifndef BOARD_SW3_GPIO
#define BOARD_SW3_GPIO GPIOD
#endif
#ifndef BOARD_SW3_PORT
#define BOARD_SW3_PORT PORTD
#endif
#ifndef BOARD_SW3_GPIO_PIN
#define BOARD_SW3_GPIO_PIN 6U
#endif
#define BOARD_SW3_IRQ PORTD_IRQn
#define BOARD_SW3_IRQ_HANDLER PORTD_IRQHandler
#define BOARD_SW3_NAME "SW3"

/* Board led color mapping */
#define LOGIC_LED_ON 0U
#define LOGIC_LED_OFF 1U
#define BOARD_LED_RED1_GPIO GPIOC
#define BOARD_LED_RED1_GPIO_PORT PORTC
#define BOARD_LED_RED1_GPIO_PIN 10U
#define BOARD_LED_GREEN1_GPIO GPIOC
#define BOARD_LED_GREEN1_GPIO_PORT PORTC
#define BOARD_LED_GREEN1_GPIO_PIN 11U
#ifndef BOARD_LED_YELLOW_GPIO
#define BOARD_LED_YELLOW_GPIO GPIOC
#endif
#define BOARD_LED_YELLOW_GPIO_PORT PORTC
#ifndef BOARD_LED_YELLOW_GPIO_PIN
#define BOARD_LED_YELLOW_GPIO_PIN 12U
#endif
#ifndef BOARD_LED_ORANGE_GPIO
#define BOARD_LED_ORANGE_GPIO GPIOC
#endif
#define BOARD_LED_ORANGE_GPIO_PORT PORTC
#ifndef BOARD_LED_ORANGE_GPIO_PIN
#define BOARD_LED_ORANGE_GPIO_PIN 13U
#endif
#define BOARD_LED_RED2_GPIO GPIOD
#define BOARD_LED_RED2_GPIO_PORT PORTD
#define BOARD_LED_RED2_GPIO_PIN 16U
#define BOARD_LED_GREEN2_GPIO GPIOD
#define BOARD_LED_GREEN2_GPIO_PORT PORTD
#define BOARD_LED_GREEN2_GPIO_PIN 15U
#ifndef BOARD_LED_BLUE_GPIO
#define BOARD_LED_BLUE_GPIO GPIOB
#endif
#define BOARD_LED_BLUE_GPIO_PORT PORTB
#ifndef BOARD_LED_BLUE_GPIO_PIN
#define BOARD_LED_BLUE_GPIO_PIN 5U
#endif

#define LED_RED1_INIT(output)                                                  \
    GPIO_PinWrite(BOARD_LED_RED1_GPIO, BOARD_LED_RED1_GPIO_PIN, output); \
    BOARD_LED_RED1_GPIO->PDDR |= (1U << BOARD_LED_RED1_GPIO_PIN) /*!< Enable target LED_RED1 */
#define LED_RED1_ON() \
    GPIO_PortClear(BOARD_LED_RED1_GPIO, 1U << BOARD_LED_RED1_GPIO_PIN) /*!< Turn on target LED_RED1 */
#define LED_RED1_OFF() \
    GPIO_PortSet(BOARD_LED_RED1_GPIO, 1U << BOARD_LED_RED1_GPIO_PIN) /*!< Turn off target LED_RED1 */
#define LED_RED1_TOGGLE() \
    GPIO_PortToggle(BOARD_LED_RED1_GPIO, 1U << BOARD_LED_RED1_GPIO_PIN) /*!< Toggle on target LED_RED1 */

#define LED_GREEN1_INIT(output)                                                    \
    GPIO_PinWrite(BOARD_LED_GREEN1_GPIO, BOARD_LED_GREEN1_GPIO_PIN, output); \
    BOARD_LED_GREEN1_GPIO->PDDR |= (1U << BOARD_LED_GREEN1_GPIO_PIN) /*!< Enable target LED_GREEN1 */
#define LED_GREEN1_ON() \
    GPIO_PortClear(BOARD_LED_GREEN1_GPIO, 1U << BOARD_LED_GREEN1_GPIO_PIN) /*!< Turn on target LED_GREEN1 */
#define LED_GREEN1_OFF() \
    GPIO_PortSet(BOARD_LED_GREEN1_GPIO, 1U << BOARD_LED_GREEN1_GPIO_PIN) /*!< Turn off target LED_GREEN1 */
#define LED_GREEN1_TOGGLE() \
    GPIO_PortToggle(BOARD_LED_GREEN1_GPIO, 1U << BOARD_LED_GREEN1_GPIO_PIN) /*!< Toggle on target LED_GREEN1 */

#define LED_YELLOW_INIT(output)                                                    \
    GPIO_PinWrite(BOARD_LED_YELLOW_GPIO, BOARD_LED_YELLOW_GPIO_PIN, output); \
    BOARD_LED_YELLOW_GPIO->PDDR |= (1U << BOARD_LED_YELLOW_GPIO_PIN) /*!< Enable target LED_YELLOW */
#define LED_YELLOW_ON() \
    GPIO_PortClear(BOARD_LED_YELLOW_GPIO, 1U << BOARD_LED_YELLOW_GPIO_PIN) /*!< Turn on target LED_YELLOW */
#define LED_YELLOW_OFF() \
    GPIO_PortSet(BOARD_LED_YELLOW_GPIO, 1U << BOARD_LED_YELLOW_GPIO_PIN) /*!< Turn off target LED_YELLOW */
#define LED_YELLOW_TOGGLE() \
    GPIO_PortToggle(BOARD_LED_YELLOW_GPIO, 1U << BOARD_LED_YELLOW_GPIO_PIN) /*!< Toggle on target LED_YELLOW */

#define LED_ORANGE_INIT(output)                                                    \
    GPIO_PinWrite(BOARD_LED_ORANGE_GPIO, BOARD_LED_ORANGE_GPIO_PIN, output); \
    BOARD_LED_ORANGE_GPIO->PDDR |= (1U << BOARD_LED_ORANGE_GPIO_PIN) /*!< Enable target LED_ORANGE */
#define LED_ORANGE_ON() \
    GPIO_PortClear(BOARD_LED_ORANGE_GPIO, 1U << BOARD_LED_ORANGE_GPIO_PIN) /*!< Turn on target LED_ORANGE */
#define LED_ORANGE_OFF() \
    GPIO_PortSet(BOARD_LED_ORANGE_GPIO, 1U << BOARD_LED_ORANGE_GPIO_PIN) /*!< Turn off target LED_ORANGE */
#define LED_ORANGE_TOGGLE() \
    GPIO_PortToggle(BOARD_LED_ORANGE_GPIO, 1U << BOARD_LED_ORANGE_GPIO_PIN) /*!< Toggle on target LED_ORANGE */

#define LED_RED2_INIT(output)                                                  \
    GPIO_PinWrite(BOARD_LED_RED2_GPIO, BOARD_LED_RED2_GPIO_PIN, output); \
    BOARD_LED_RED2_GPIO->PDDR |= (1U << BOARD_LED_RED2_GPIO_PIN) /*!< Enable target LED_RED2 */
#define LED_RED2_ON() \
    GPIO_PortClear(BOARD_LED_RED2_GPIO, 1U << BOARD_LED_RED2_GPIO_PIN) /*!< Turn on target LED_RED2 */
#define LED_RED2_OFF() \
    GPIO_PortSet(BOARD_LED_RED2_GPIO, 1U << BOARD_LED_RED2_GPIO_PIN) /*!< Turn off target LED_RED2 */
#define LED_RED2_TOGGLE() \
    GPIO_PortToggle(BOARD_LED_RED2_GPIO, 1U << BOARD_LED_RED2_GPIO_PIN) /*!< Toggle on target LED_RED2 */

#define LED_GREEN2_INIT(output)                                                    \
    GPIO_PinWrite(BOARD_LED_GREEN2_GPIO, BOARD_LED_GREEN2_GPIO_PIN, output); \
    BOARD_LED_GREEN2_GPIO->PDDR |= (1U << BOARD_LED_GREEN2_GPIO_PIN) /*!< Enable target LED_GREEN2 */
#define LED_GREEN2_ON() \
    GPIO_PortClear(BOARD_LED_GREEN2_GPIO, 1U << BOARD_LED_GREEN2_GPIO_PIN) /*!< Turn on target LED_GREEN2 */
#define LED_GREEN2_OFF() \
    GPIO_PortSet(BOARD_LED_GREEN2_GPIO, 1U << BOARD_LED_GREEN2_GPIO_PIN) /*!< Turn off target LED_GREEN2 */
#define LED_GREEN2_TOGGLE() \
    GPIO_PortToggle(BOARD_LED_GREEN2_GPIO, 1U << BOARD_LED_GREEN2_GPIO_PIN) /*!< Toggle on target LED_GREEN2 */

#define LED_BLUE_INIT(output)                                                  \
    GPIO_PinWrite(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PIN, output); \
    BOARD_LED_BLUE_GPIO->PDDR |= (1U << BOARD_LED_BLUE_GPIO_PIN) /*!< Enable target LED_BLUE */
#define LED_BLUE_ON() \
    GPIO_PortClear(BOARD_LED_BLUE_GPIO, 1U << BOARD_LED_BLUE_GPIO_PIN) /*!< Turn on target LED_BLUE */
#define LED_BLUE_OFF() \
    GPIO_PortSet(BOARD_LED_BLUE_GPIO, 1U << BOARD_LED_BLUE_GPIO_PIN) /*!< Turn off target LED_BLUE */
#define LED_BLUE_TOGGLE() \
    GPIO_PortToggle(BOARD_LED_BLUE_GPIO, 1U << BOARD_LED_BLUE_GPIO_PIN) /*!< Toggle on target LED_BLUE */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * API
 ******************************************************************************/

void BOARD_InitDebugConsole(void);


#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _BOARD_H_ */
