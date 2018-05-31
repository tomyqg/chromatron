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

#ifndef _VM_WIFI_CMD_H
#define _VM_WIFI_CMD_H

#include "vm_core.h"

typedef struct __attribute__((packed)){
    uint32_t vm_id;
} wifi_msg_reset_vm_t;
#define WIFI_DATA_ID_RESET_VM          0x20

typedef struct __attribute__((packed)){
    uint32_t vm_id;
    uint8_t chunk[64];
} wifi_msg_load_vm_t;
#define WIFI_DATA_ID_LOAD_VM           0x21


typedef struct __attribute__((packed)){
    vm_info_t vm_info[VM_MAX_VMS];
} wifi_msg_vm_info_t;
#define WIFI_DATA_ID_VM_INFO           0x22

#define WIFI_DATA_ID_RUN_VM             0x26



#endif


