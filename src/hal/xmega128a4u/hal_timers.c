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

#include "os_irq.h"
#include "timers.h"
#include "power.h"
#include "hal_timers.h"

/*

Base clock 32 MHz
Prescaler /64
Timer: 500 KHz (2 uS resolution)

Overflow rate: 7.63 Hz


On overflow: increment microseconds counter by 131072.


*/

static volatile uint64_t microseconds;
static volatile uint32_t alarm_microseconds;


void hal_timer_v_init( void ){

    // enable overflow interrupt and set priority level to high
    TCC1.INTCTRLA |= TC_OVFINTLVL_HI_gc;

    // start timer, prescaler /64
    TCC1.CTRLA |= TC_CLKSEL_DIV64_gc;
}

// Return TRUE if any timers on the IO clock are running.
// The async timer is a different clock domain, so it does not count.
bool tmr_b_io_timers_running( void ){

    // timers are always running

    return TRUE;
}

uint64_t tmr_u64_get_system_time_us( void ){

    // full 64 bit system time is:
    // microseconds + current timer count * 2


    // read timer twice.
    // if second read is lower than the first,
    // the timer has rolled over. in that case,
    // repeat the procedure.
    

    uint16_t timer1, timer2;
    uint64_t current_microseconds;

    do{
        ATOMIC;

        timer1 = TCC1.CNT;
        timer2 = TCC1.CNT;
        current_microseconds = microseconds;

        END_ATOMIC;

    } while( timer1 > timer2 );

    return current_microseconds + timer2;
}


static void disarm_alarm( void ){

    TCC1.INTCTRLB &= ~TC1_CCAINTLVL_gm;
}

static void arm_alarm( void ){

    TCC1.INTCTRLB |= TC_CCAINTLVL_HI_gc;
}


int8_t tmr_i8_set_alarm_microseconds( int64_t alarm ){

    int64_t now = tmr_u64_get_system_time_us();

    int64_t time_to_alarm = alarm - now;

    // check if alarm has already passed, or it is too soon
    // to set the alarm (128 us)
    if( time_to_alarm <= 128 ){

        return -1;
    }
    // check if alarm is too long (1 minute)
    else if( time_to_alarm > 60000000 ){

        time_to_alarm = 60000000;
    }

    ATOMIC;

    disarm_alarm();

    // set alarm
    alarm_microseconds = time_to_alarm;

    // check if within current timer range
    if( alarm_microseconds < 131072 ){

        // set alarm
        TCC1.CCA = TCC1.CNT + MICROSECONDS_TO_TIMER_TICKS( alarm_microseconds );

        arm_alarm();    
    }

    END_ATOMIC;

    return 0;
}

void tmr_v_cancel_alarm( void ){

    ATOMIC;
    disarm_alarm();

    alarm_microseconds = 0;
    END_ATOMIC;
}

// return true if alarm is armed
bool tmr_b_alarm_armed( void ){

    ATOMIC;
    bool armed = alarm_microseconds == 0;
    END_ATOMIC;

    return armed;
}


static void alarm( void ){

    tmr_v_cancel_alarm();

    #ifdef ENABLE_POWER
    pwr_v_wake_up( PWR_WAKE_SOURCE_TIMER );
    #endif
}

ISR(TCC1_OVF_vect){
// OS_IRQ_BEGIN(TCC1_OVF_vect);
// ^^^ Do not use the OS_IRQ macros in this interrupt.
// This is because the IRQ timing will call into
// the system time functions, and the timer rollover
// will yield an incorrect timing update.

    microseconds += 131072;

    // check if we need to arm the alarm
    if( alarm_microseconds > 0 ){

        if( alarm_microseconds >= 131072 ){

            alarm_microseconds -= 131072;

            // check if alarm is very very soon
            if( alarm_microseconds < 32 ){

                // alarm now
                alarm();
            }
            else{

                // arm timer compare for alarm
                TCC1.CCA = TCC1.CNT + MICROSECONDS_TO_TIMER_TICKS( alarm_microseconds );

                arm_alarm();    
            }
        }

    }

// OS_IRQ_END();
}


ISR(TCC1_CCA_vect){
OS_IRQ_BEGIN(TCC1_CCA_vect);

   alarm();

OS_IRQ_END();
}

