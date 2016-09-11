/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "platform_sleep.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/*
BCM9WCDUSI09 platform pin definitions ...
+-----------------------------------------------------------------------------------------+
|Pin |   Pin Name on    |    Module     | STM32| Peripheral  |    Board     | Peripheral  |
| #  |      Module      |  GPIO Alias   | Port | Available   |  Connection  |   Alias     |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 27 | MICRO_WKUP       | WICED_GPIO_1  | A  0 | GPIO        |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
|  8 | MICRO_GPIO_0     | WICED_GPIO_2  | A  1 | GPIO        | BUTTON SW2   |             |
|    |                  |               |      | TIM2_CH2    |              |             |
|    |                  |               |      | TIM5_CH2    |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
|  7 | MICRO_GPIO_1     | WICED_GPIO_3  | A  2 | ADC123_IN2  |              |             |
|    |                  |               |      | GPIO        | BUTTON SW1   |             |
|    |                  |               |      | TIM2_CH3    |              |             |
|    |                  |               |      | TIM5_CH3    |              |             |
|    |                  |               |      | TIM9_CH1    |              |             |
|    |                  |               |      | USART2_TX   |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
|  6 | MICRO_GPIO_2     | WICED_GPIO_4  | A  3 | ADC123_IN3  |              | WICED_ADC_3 |
|    |                  |               |      | GPIO        | THERMISTOR   |             |
|    |                  |               |      | TIM2_CH4    |              |             |
|    |                  |               |      | TIM5_CH4    |              |             |
|    |                  |               |      | TIM9_CH2    |              |             |
|    |                  |               |      | UART2_RX    |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 22 | MICRO_SPI_SSN    | WICED_GPIO_5  | A  4 | ADC12_IN4   |              |             |
|    |                  |               |      | DAC1_OUT    |              |             |
|    |                  |               |      | GPIO        |              |             |
|    |                  |               |      | I2S3_WS     |              |             |
|    |                  |               |      | SPI1_NSS    |              |             |
|    |                  |               |      | SPI3_NSS    |              |             |
|    |                  |               |      | USART2_CK   |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 24 | MICRO_SPI_SCK    | WICED_GPIO_6  | A  5 | ADC12_IN5   |              |             |
|    |                  |               |      | DAC2_OUT    |              |             |
|    |                  |               |      | GPIO        |              |             |
|    |                  |               |      | SPI1_SCK    |              |             |
|    |                  |               |      | TIM2_CH1_ETR|              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 25 | MICRO_SPI_MISO   | WICED_GPIO_7  | A  6 | ADC12_IN6   |              |             |
|    |                  |               |      | GPIO        |              |             |
|    |                  |               |      | SPI1_MISO   |              |             |
|    |                  |               |      | TIM1_BKIN   |              |             |
|    |                  |               |      | TIM3_CH1    |              |             |
|    |                  |               |      | TIM8_BKIN   |              |             |
|    |                  |               |      | TIM13_CH1   |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 23 | MICRO_SPI_MOSI   | WICED_GPIO_8  | A  7 | ADC12_IN7   |              |             |
|    |                  |               |      | GPIO        |              |             |
|    |                  |               |      | SPI1_MOSI   |              |             |
|    |                  |               |      | TIM1_CH1N   |              |             |
|    |                  |               |      | TIM3_CH2    |              |             |
|    |                  |               |      | TIM8_CH1N   |              |             |
|    |                  |               |      | TIM14_CH1   |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 39 | MICRO_UART_TX    | WICED_GPIO_9  | A  9 | GPIO        |              |             |
|    |                  |               |      | I2C3_SMBA   |              |             |
|    |                  |               |      | TIM1_CH2    |              |             |
|    |                  |               |      | USART1_TX   |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 38 | MICRO_UART_RX    | WICED_GPIO_10 | A 10 | GPIO        |              |             |
|    |                  |               |      | TIM1_CH3    |              |             |
|    |                  |               |      | USART1_RX   |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 44 | MICRO_JTAG_TMS   | WICED_GPIO_11 | A 13 | GPIO        |              |             |
|    |                  |               |      | JTMS-SWDIO  |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 40 | MICRO_JTAG_TCK   | WICED_GPIO_12 | A 14 | GPIO        |              |             |
|    |                  |               |      | JTCK-SWCLK  |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 43 | MICRO_JTAG_TDI   | WICED_GPIO_13 | A 15 | GPIO        |              |             |
|    |                  |               |      | JTDI        |              |             |
|    |                  |               |      | I2S3_WS     |              |             |
|    |                  |               |      | SPI1_NSS    |              |             |
|    |                  |               |      | SPI3_NSS    |              |             |
|    |                  |               |      | TIM2_CH1_ETR|              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 41 | MICRO_JTAG_TDO   | WICED_GPIO_14 | B  3 | GPIO        |              |             |
|    |                  |               |      | JTDO        |              |             |
|    |                  |               |      | SPI1_SCK    |              |             |
|    |                  |               |      | SPI3_SCK    |              |             |
|    |                  |               |      | I2S3_SCK    |              |             |
|    |                  |               |      | TIM2_CH2    |              |             |
|    |                  |               |      | TRACESWO    |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 42 | MICRO_JTAG_TRST  | WICED_GPIO_15 | B  4 | GPIO        |              |             |
|    |                  |               |      | NJTRST      |              |             |
|    |                  |               |      | SPI1_MISO   |              |             |
|    |                  |               |      | SPI3_MISO   |              |             |
|    |                  |               |      | TIM3_CH1    |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
|  3 | MICRO_GPIO_5     | WICED_GPIO_16 | B  5 | GPIO        |              |             |
|    |                  |               |      | I2C1_SMBA   |              |             |
|    |                  |               |      | I2S3_SD     |              |             |
|    |                  |               |      | SPI1_MOSI   |              |             |
|    |                  |               |      | SPI3_MOSI   |              |             |
|    |                  |               |      | TIM3_CH2    |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
|  5 | MICRO_GPIO_3     | WICED_GPIO_17 | B  6 | GPIO        | LED D1       |             |
|    |                  |               |      | TIM4_CH1    |              | WICED_PWM_1 |
|    |                  |               |      | I2C1_SCL    |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
|  4 | MICRO_GPIO_4     | WICED_GPIO_18 | B  7 | GPIO        | LED D2       |             |
|    |                  |               |      | TIM4_CH2    |              | WICED_PWM_2 |
|    |                  |               |      | I2C1_SDA    |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 51 | MICRO_USB_HS_DN  | WICED_GPIO_19 | B 14 | GPIO        |              |             |
|    |                  |               |      | USB_HS_DN   |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 52 | MICRO_USB_HS_DP  | WICED_GPIO_20 | B 15 | GPIO        |              |             |
|    |                  |               |      | USB_HS_DP   |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
|  2 | MICRO_GPIO_6     | WICED_GPIO_21 | C  2 | ADC123_IN12 |              |             |
|    |                  |               |      | GPIO        |              |             |
|    |                  |               |      | SPI2_MISO   |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
|  1 | MICRO_GPIO_7     | WICED_GPIO_22 | C  3 | ADC123_IN13 |              |             |
|    |                  |               |      | GPIO        |              |             |
|    |                  |               |      | I2S2_SD     |              |             |
|    |                  |               |      | SPI2_MOSI   |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 54 | MICRO_GPIO_8     | WICED_GPIO_23 | C  5 | ADC12_IN15  |              |             |
|    |                  |               |      | GPIO        |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
| 53 | MICRO_GPIO_9     | WICED_GPIO_24 | C  7 | GPIO        |              |             |
|    |                  |               |      | I2S3_MCK    |              |             |
|    |                  |               |      | TIM8_CH2    |              |             |
|    |                  |               |      | TIM3_CH2    |              |             |
|----+------------------+---------------+------+-------------+--------------+-------------|
Notes
1. These mappings are defined in <WICED-SDK>/Platform/BCM9WCDUSI09/platform.c
2. STM32F2xx Datasheet  -> http://www.st.com/web/en/resource/technical/document/datasheet/CD00237391.pdf
3. STM32F2xx Ref Manual -> http://www.st.com/web/en/resource/technical/document/reference_manual/CD00225773.pdf
*/


typedef enum
{
    WICED_GPIO_1,
    WICED_GPIO_2,
    WICED_GPIO_3,
    WICED_GPIO_4,
    WICED_GPIO_5,
    WICED_GPIO_6,
    WICED_GPIO_7,
    WICED_GPIO_8,
    WICED_GPIO_9,
    WICED_GPIO_10,
    WICED_GPIO_11,
    WICED_GPIO_12,
    WICED_GPIO_13,
    WICED_GPIO_14,
    WICED_GPIO_15,
    WICED_GPIO_16,
    WICED_GPIO_17,
    WICED_GPIO_18,
    WICED_GPIO_19,
    WICED_GPIO_20,
    WICED_GPIO_21,
    WICED_GPIO_22,
    WICED_GPIO_23,
    WICED_GPIO_24,
    WICED_GPIO_MAX, /* Denotes the total number of GPIO aliases. Not a valid GPIO alias */
} wiced_gpio_t;

typedef enum
{
    WICED_SPI_1,
    WICED_SPI_MAX, /* Denotes the total number of SPI port aliases. Not a valid SPI alias */
} wiced_spi_t;

typedef enum
{
    WICED_I2C_NONE = 0xFF
} wiced_i2c_t;

typedef enum
{
    WICED_PWM_1,
    WICED_PWM_2,
    WICED_PWM_3,
    WICED_PWM_4,
    WICED_PWM_5,
    WICED_PWM_6,
    WICED_PWM_7,
    WICED_PWM_8,
    WICED_PWM_9,
    WICED_PWM_MAX, /* Denotes the total number of PWM port aliases. Not a valid PWM alias */
} wiced_pwm_t;

typedef enum
{
    WICED_ADC_1,
    WICED_ADC_2,
    WICED_ADC_3,
    WICED_ADC_MAX, /* Denotes the total number of ADC port aliases. Not a valid ADC alias */
} wiced_adc_t;

typedef enum
{
    WICED_UART_1,
    WICED_UART_2,
    WICED_UART_MAX, /* Denotes the total number of UART port aliases. Not a valid UART alias */
} wiced_uart_t;

#define WICED_PLATFORM_INCLUDES_SPI_FLASH
#define WICED_SPI_FLASH_CS  (WICED_GPIO_5)
/*      WICED_SPI_FLASH_MOSI WICED_GPIO_8 */
/*      WICED_SPI_FLASH_MISO WICED_GPIO_7 */
/*      WICED_SPI_FLASH_CLK  WICED_GPIO_6 */

/* Components connected to external I/Os*/
#define WICED_LED1       (WICED_GPIO_17)
#define WICED_LED2       (WICED_GPIO_18)
#define WICED_BUTTON1    (WICED_GPIO_3)
#define WICED_BUTTON2    (WICED_GPIO_2)
#define WICED_THERMISTOR (WICED_GPIO_4)

/* I/O connection <-> Peripheral Connections */
#define WICED_LED1_JOINS_PWM       (WICED_PWM_1)
#define WICED_LED2_JOINS_PWM       (WICED_PWM_2)
#define WICED_THERMISTOR_JOINS_ADC (WICED_ADC_3)

/* WLAN Powersave Clock Source
 * The WLAN sleep clock can ONLY be driven by a Timer/PWM
 * The STM32 can *NOT* be put into MCU powersave mode or the PWM output will be disabled
 */
#define WICED_WLAN_POWERSAVE_CLOCK_SOURCE WICED_WLAN_POWERSAVE_CLOCK_IS_PWM

#ifdef __cplusplus
} /*extern "C" */
#endif
