/*
// <license>
// 
//     This file is part of the Sapphire Operating System.
// 
//     Copyright (C) 2013-2018  Jeremy Billheimer
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


#include "cpu.h"

#include "system.h"
#include "target.h"

#include "hal_io.h"


void io_v_init( void ){

    LL_GPIO_InitTypeDef GPIO_InitStruct;

    /* GPIO Ports Clock Enable */
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOH);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);

    /**/
    LL_GPIO_ResetOutputPin(LED0_GPIO_Port, LED0_Pin);

    /**/
    LL_GPIO_ResetOutputPin(LED1_GPIO_Port, LED1_Pin);

    /**/
    LL_GPIO_ResetOutputPin(LED2_GPIO_Port, LED2_Pin);

    /**/
    LL_GPIO_ResetOutputPin(WIFI_PD_GPIO_Port, WIFI_PD_Pin);

    /**/
    LL_GPIO_ResetOutputPin(WIFI_RST_GPIO_Port, WIFI_RST_Pin);

    /**/
    LL_GPIO_ResetOutputPin(WIFI_BOOT_R_GPIO_Port, WIFI_BOOT_R_Pin);

    /**/
    LL_GPIO_ResetOutputPin(WIFI_SS_GPIO_Port, WIFI_SS_Pin);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_13;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_14;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_15;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LED0_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LED0_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LED1_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LED2_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LED2_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_5;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = GPIO4_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIO4_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = GPIO5_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIO5_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_12;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_14;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_8;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_8;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_12;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_15;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = WIFI_PD_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(WIFI_PD_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = WIFI_RST_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(WIFI_RST_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = WIFI_BOOT_R_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(WIFI_BOOT_R_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin = WIFI_SS_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(WIFI_SS_GPIO_Port, &GPIO_InitStruct);


    // set all other pins to inputs pulled down
    for( uint8_t i = 0; i < IO_PIN_COUNT; i++ ){

        io_v_set_mode( i, IO_MODE_INPUT_PULLDOWN );
    }
}

uint8_t io_u8_get_board_rev( void ){

    return 0;
}


void io_v_set_mode( uint8_t pin, io_mode_t8 mode ){

	LL_GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_TypeDef *GPIOx;
    GPIO_InitStruct.Mode        = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Speed       = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType  = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull        = LL_GPIO_PULL_NO;

	switch( pin ){
        case IO_PIN_0:
        	GPIOx = IO_PIN0_PORT;
        	GPIO_InitStruct.Pin = IO_PIN0_PIN;
            break;

        case IO_PIN_1:
        	GPIOx = IO_PIN1_PORT;
        	GPIO_InitStruct.Pin = IO_PIN1_PIN;
            break;

        case IO_PIN_2:
        	GPIOx = IO_PIN2_PORT;
        	GPIO_InitStruct.Pin = IO_PIN2_PIN;
            break;

        case IO_PIN_3:
        	GPIOx = IO_PIN3_PORT;
        	GPIO_InitStruct.Pin = IO_PIN3_PIN;
            break;

        case IO_PIN_4:
        	GPIOx = IO_PIN4_PORT;
        	GPIO_InitStruct.Pin = IO_PIN4_PIN;
            break;

        case IO_PIN_5:
        	GPIOx = IO_PIN5_PORT;
        	GPIO_InitStruct.Pin = IO_PIN5_PIN;
            break;

        default:
            return;
            break;
    }


    if( mode >= IO_MODE_OUTPUT ){

    	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;

        if( mode == IO_MODE_OUTPUT_OPEN_DRAIN ){

        	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
        }
        else{

        	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        }
    }
    else{

    	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;

        if( mode == IO_MODE_INPUT_PULLUP ){

        	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
        }
        else if( mode == IO_MODE_INPUT_PULLDOWN ){

        	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
        }
    }

    LL_GPIO_Init( GPIOx, &GPIO_InitStruct );
}


io_mode_t8 io_u8_get_mode( uint8_t pin ){

	GPIO_TypeDef *GPIOx;
	uint32_t gpio_pin;

	switch( pin ){
        case IO_PIN_0:
        	GPIOx = IO_PIN0_PORT;
        	gpio_pin = IO_PIN0_PIN;
            break;

        case IO_PIN_1:
        	GPIOx = IO_PIN1_PORT;
        	gpio_pin = IO_PIN1_PIN;
            break;

        case IO_PIN_2:
        	GPIOx = IO_PIN2_PORT;
        	gpio_pin = IO_PIN2_PIN;
            break;

        case IO_PIN_3:
        	GPIOx = IO_PIN3_PORT;
        	gpio_pin = IO_PIN3_PIN;
            break;

        case IO_PIN_4:
        	GPIOx = IO_PIN4_PORT;
        	gpio_pin = IO_PIN4_PIN;
            break;

        case IO_PIN_5:
        	GPIOx = IO_PIN5_PORT;
        	gpio_pin = IO_PIN5_PIN;
            break;

        default:
            return IO_MODE_INPUT;
            break;
    }


    if( LL_GPIO_GetPinMode( GPIOx, gpio_pin ) == LL_GPIO_MODE_OUTPUT ){

    	if( LL_GPIO_GetPinOutputType( GPIOx, gpio_pin ) == LL_GPIO_OUTPUT_PUSHPULL ){

    		return IO_MODE_OUTPUT;
    	}
    	else{

    		return IO_MODE_OUTPUT_OPEN_DRAIN;
    	}
    }
    else{

    	if( LL_GPIO_GetPinPull( GPIOx, gpio_pin ) == LL_GPIO_PULL_UP ){

    		return IO_MODE_INPUT_PULLUP;
    	}
    	else if( LL_GPIO_GetPinPull( GPIOx, gpio_pin ) == LL_GPIO_PULL_DOWN ){

    		return IO_MODE_INPUT_PULLDOWN;
    	}
    	else{

    		return IO_MODE_INPUT;	
    	}
    }

    return IO_MODE_INPUT;
}

void io_v_digital_write( uint8_t pin, bool state ){

	GPIO_TypeDef *GPIOx;
	uint32_t gpio_pin;

	switch( pin ){
        case IO_PIN_0:
        	GPIOx = IO_PIN0_PORT;
        	gpio_pin = IO_PIN0_PIN;
            break;

        case IO_PIN_1:
        	GPIOx = IO_PIN1_PORT;
        	gpio_pin = IO_PIN1_PIN;
            break;

        case IO_PIN_2:
        	GPIOx = IO_PIN2_PORT;
        	gpio_pin = IO_PIN2_PIN;
            break;

        case IO_PIN_3:
        	GPIOx = IO_PIN3_PORT;
        	gpio_pin = IO_PIN3_PIN;
            break;

        case IO_PIN_4:
        	GPIOx = IO_PIN4_PORT;
        	gpio_pin = IO_PIN4_PIN;
            break;

        case IO_PIN_5:
        	GPIOx = IO_PIN5_PORT;
        	gpio_pin = IO_PIN5_PIN;
            break;

        default:
            return;
            break;
    }

    if( state ){

    	LL_GPIO_SetOutputPin( GPIOx, gpio_pin );
    }
    else{

    	LL_GPIO_ResetOutputPin( GPIOx, gpio_pin );
    }
}

bool io_b_digital_read( uint8_t pin ){

	GPIO_TypeDef *GPIOx;
	uint32_t gpio_pin;

	switch( pin ){
        case IO_PIN_0:
        	GPIOx = IO_PIN0_PORT;
        	gpio_pin = IO_PIN0_PIN;
            break;

        case IO_PIN_1:
        	GPIOx = IO_PIN1_PORT;
        	gpio_pin = IO_PIN1_PIN;
            break;

        case IO_PIN_2:
        	GPIOx = IO_PIN2_PORT;
        	gpio_pin = IO_PIN2_PIN;
            break;

        case IO_PIN_3:
        	GPIOx = IO_PIN3_PORT;
        	gpio_pin = IO_PIN3_PIN;
            break;

        case IO_PIN_4:
        	GPIOx = IO_PIN4_PORT;
        	gpio_pin = IO_PIN4_PIN;
            break;

        case IO_PIN_5:
        	GPIOx = IO_PIN5_PORT;
        	gpio_pin = IO_PIN5_PIN;
            break;

        default:
            return FALSE;
            break;
    }

    uint32_t port = LL_GPIO_ReadInputPort( GPIOx );

    return ( port & ( 1 << gpio_pin ) ) != 0;
}

bool io_b_button_down( void ){

    return FALSE;
}

void io_v_disable_jtag( void ){

}

void io_v_enable_interrupt(
    uint8_t int_number,
    io_int_handler_t handler,
    io_int_mode_t8 mode )
{

}

void io_v_disable_interrupt( uint8_t int_number )
{

}
