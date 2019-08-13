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


#ifndef _HAL_CPU_H	
#define _HAL_CPU_H

#include "bool.h"

#define ENABLE_INTERRUPTS __enable_irq()
#define DISABLE_INTERRUPTS __disable_irq()

#define ATOMIC
#define END_ATOMIC

#define FLASH_STRING(x) x
#define FLASH_STRING_T const char*

#define FW_INFO_SECTION __attribute__ ((section (".fwinfo"), used))
#define NON_CACHEABLE __attribute__ ((section (".non_cacheable")))
#define MEMORY_HEAP __attribute__ ((section (".memory_heap")))

#define hal_cpu_v_delay_us( us )


#endif