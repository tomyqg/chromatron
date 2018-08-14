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

#ifndef _VM_RUNNER_H
#define _VM_RUNNER_H

#include "list.h"
#include "gfx_lib.h"
#include "vm_core.h"
#include "wifi_cmd.h"
#include "vm_wifi_cmd.h"

#define VM_RUNNER_MAX_SIZE 		8192

#define VM_RUNNER_THREAD_RATE 	10

void vm_v_init( void );
void vm_v_run_faders( void );
void vm_v_run_vm( void );
void vm_v_process( void );
void vm_v_send_info( void );
void vm_v_reset(  uint8_t vm_index );
int8_t vm_i8_load( uint8_t *data, uint16_t len, uint8_t vm_index );
int8_t vm_i8_start( uint32_t vm_index );
void vm_v_request( void );
void vm_v_get_info( uint8_t index, vm_info_t *info );
uint16_t vm_u16_get_fader_time( void );
uint16_t vm_u16_get_total_size( void );

// int8_t vm_i8_get_frame_sync( uint8_t index, wifi_msg_vm_frame_sync_t *sync );
// uint8_t vm_u8_set_frame_sync( wifi_msg_vm_frame_sync_t *sync );
// uint16_t vm_u16_get_frame_number( void );

void vm_v_start_frame_sync( uint8_t index, wifi_msg_vm_frame_sync_t *sync );
void vm_v_frame_sync_data( uint8_t index, wifi_msg_vm_sync_data_t *sync );
void vm_v_frame_sync_done( uint8_t index, wifi_msg_vm_sync_done_t *sync );

int32_t vm_i32_get_reg( uint8_t addr, uint8_t vm_index );
void vm_v_set_reg( uint8_t addr, int32_t data, uint8_t vm_index );

void vm_v_request_frame_data( uint8_t index );
void vm_v_run_fader( void );

#endif
