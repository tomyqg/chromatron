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


#include "system.h"
#include "hal_cpu.h"
#include "os_irq.h"
#include "hal_watchdog.h"
#include "watchdog.h"

#define WDG_TIMER_STEP 5000

#define WDG_TIMEOUT 16

static uint8_t wdg_timer;


void wdg_v_reset( void ){

    ATOMIC;
    wdg_timer = WDG_TIMEOUT;
    END_ATOMIC;
}

void wdg_v_enable( wdg_timeout_t8 timeout, wdg_flags_t8 flags ){

    ATOMIC;

    wdt_reset();

    ENABLE_CONFIG_CHANGE;

    WDT.CTRL = timeout | WDT_ENABLE_bm | WDT_CEN_bm;

    // set timer compare match B
    TCC1.CCB = TCC1.CNT + WDG_TIMER_STEP;
    TCC1.INTCTRLB |= TC_CCBINTLVL_HI_gc;

    END_ATOMIC;
}

void wdg_v_disable( void ){

    ATOMIC;

    wdt_reset();

    ENABLE_CONFIG_CHANGE;

    WDT.CTRL = WDT_CEN_bm;

    END_ATOMIC;
}

#ifndef BOOTLOADER
ISR(TCC1_CCB_vect){
OS_IRQ_BEGIN(TCC1_CCB_vect);

    wdt_reset();

    // increment step
    TCC1.CCB = TCC1.CNT + WDG_TIMER_STEP;

    wdg_timer--;

    if( wdg_timer == 0 ){

        ASSERT( 0 );
    }

OS_IRQ_END();
}
#endif
