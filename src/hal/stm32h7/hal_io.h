/*
// <license>
// 
//     This file is part of the Sapphire Operating System.
// 
//     Copyright (C) 2013-2019  Jeremy Billheimer
// 
// 
//     This program is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
// 
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
// 
//     You should have received a copy of the GNU General Public License
//     along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 
// </license>
 */

#ifndef _HAL_IO_H
#define _HAL_IO_H

#include "system.h"
#include "io_intf.h"
#include "cpu.h"

#include "stm32h7xx_hal.h"

#define IO_PIN_0       		0 // PA4
#define IO_PIN_1      		1 // PA5
#define IO_PIN_2       		2 // PA6
#define IO_PIN_3       		3 // PA7
#define IO_PIN_4       		4 // PB10
#define IO_PIN_5       		5 // PB11
#define IO_PIN_COUNT        6

#define IO_PIN0_PORT        GPIOA
#define IO_PIN0_PIN         GPIO_PIN_4
#define IO_PIN1_PORT        GPIOA
#define IO_PIN1_PIN         GPIO_PIN_5
#define IO_PIN2_PORT        GPIOA
#define IO_PIN2_PIN         GPIO_PIN_6
#define IO_PIN3_PORT        GPIOA
#define IO_PIN3_PIN         GPIO_PIN_7
#define IO_PIN4_PORT        GPIOB
#define IO_PIN4_PIN         GPIO_PIN_10
#define IO_PIN5_PORT        GPIOB
#define IO_PIN5_PIN         GPIO_PIN_11


// TODO THIS IS THE WRONG PIN!!!
#define VMON_Pin GPIO_PIN_0
#define VMON_GPIO_Port GPIOC

#define PIX_CLK_3_Pin GPIO_PIN_2
#define PIX_CLK_3_GPIO_Port GPIOE
#define WIFI_BOOT_Pin GPIO_PIN_3
#define WIFI_BOOT_GPIO_Port GPIOE
#define WIFI_RST_Pin GPIO_PIN_4
#define WIFI_RST_GPIO_Port GPIOE
#define WIFI_RX_Ready_Pin GPIO_PIN_5
#define WIFI_RX_Ready_Port GPIOE
#define PIX_DAT_3_Pin GPIO_PIN_6
#define PIX_DAT_3_GPIO_Port GPIOE
#define USER_Btn_Pin GPIO_PIN_13
#define USER_Btn_GPIO_Port GPIOC
#define WIFI_PD_Pin GPIO_PIN_0
#define WIFI_PD_GPIO_Port GPIOF
#define WIFI_SS_Pin GPIO_PIN_1
#define WIFI_SS_GPIO_Port GPIOF
#define AUX_UART_RX_Pin GPIO_PIN_6
#define AUX_UART_RX_GPIO_Port GPIOF
#define PIX_CLK_4_Pin GPIO_PIN_7
#define PIX_CLK_4_GPIO_Port GPIOF
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOH
#define LED1_Pin GPIO_PIN_2
#define LED1_GPIO_Port GPIOC
#define PIX_DAT_1_Pin GPIO_PIN_3
#define PIX_DAT_1_GPIO_Port GPIOC
#define PIX_DAT_8_Pin GPIO_PIN_0
#define PIX_DAT_8_GPIO_Port GPIOA
#define PIX_DAT_7_Pin GPIO_PIN_2
#define PIX_DAT_7_GPIO_Port GPIOA
#define PIX_CLK_7_Pin GPIO_PIN_4
#define PIX_CLK_7_GPIO_Port GPIOA
#define PIX_CLK_0_Pin GPIO_PIN_5
#define PIX_CLK_0_GPIO_Port GPIOA
#define PIX_DAT_5_Pin GPIO_PIN_7
#define PIX_DAT_5_GPIO_Port GPIOA
#define PIX_DAT_2_Pin GPIO_PIN_2
#define PIX_DAT_2_GPIO_Port GPIOB
#define PIX_DAT_4_Pin GPIO_PIN_11
#define PIX_DAT_4_GPIO_Port GPIOF
#define LD3_Pin GPIO_PIN_14
#define LD3_GPIO_Port GPIOB
#define CMD_USART_TX_Pin GPIO_PIN_8
#define CMD_USART_TX_GPIO_Port GPIOD
#define CMD_USART_RX_Pin GPIO_PIN_9
#define CMD_USART_RX_GPIO_Port GPIOD
#define USB_PowerSwitchOn_Pin GPIO_PIN_6
#define USB_PowerSwitchOn_GPIO_Port GPIOG
#define USB_OverCurrent_Pin GPIO_PIN_7
#define USB_OverCurrent_GPIO_Port GPIOG
#define AUX_SPI_MOSI_Pin GPIO_PIN_6
#define AUX_SPI_MOSI_GPIO_Port GPIOC
#define AUX_SPI_MISO_Pin GPIO_PIN_7
#define AUX_SPI_MISO_GPIO_Port GPIOC
#define AUX_SPI_SCK_Pin GPIO_PIN_8
#define AUX_SPI_SCK_GPIO_Port GPIOC
#define PIX_CLK_6_Pin GPIO_PIN_8
#define PIX_CLK_6_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define AUX_UART_TX_Pin GPIO_PIN_15
#define AUX_UART_TX_GPIO_Port GPIOA
#define PIX_CLK_2_Pin GPIO_PIN_10
#define PIX_CLK_2_GPIO_Port GPIOC
#define LED0_Pin GPIO_PIN_11
#define LED0_GPIO_Port GPIOC
#define PIX_DAT_9_Pin GPIO_PIN_12
#define PIX_DAT_9_GPIO_Port GPIOC
#define PIX_CLK_1_Pin GPIO_PIN_3
#define PIX_CLK_1_GPIO_Port GPIOD
#define LED2_Pin GPIO_PIN_4
#define LED2_GPIO_Port GPIOD
#define PIX_DAT_0_Pin GPIO_PIN_7
#define PIX_DAT_0_GPIO_Port GPIOD
#define PIX_CLK_5_Pin GPIO_PIN_13
#define PIX_CLK_5_GPIO_Port GPIOG
#define PIX_DAT_6_Pin GPIO_PIN_6
#define PIX_DAT_6_GPIO_Port GPIOB
#define LD2_Pin GPIO_PIN_7
#define LD2_GPIO_Port GPIOB
#define WIFI_RXD_Pin GPIO_PIN_0
#define WIFI_RXD_GPIO_Port GPIOE
#define WIFI_TXD_Pin GPIO_PIN_1
#define WIFI_TXD_GPIO_Port GPIOE
#define I2C1_SCL_Pin GPIO_PIN_8
#define I2C1_SCL_GPIO_Port GPIOB
#define I2C1_SDA_Pin GPIO_PIN_9
#define I2C1_SDA_GPIO_Port GPIOB
#define SPI4_MOSI_Pin GPIO_PIN_6
#define SPI4_MOSI_GPIO_Port GPIOE
#define SPI4_MISO_Pin GPIO_PIN_5
#define SPI4_MISO_GPIO_Port GPIOE
#define SPI4_SCK_Pin GPIO_PIN_2
#define SPI4_SCK_GPIO_Port GPIOE


#endif
