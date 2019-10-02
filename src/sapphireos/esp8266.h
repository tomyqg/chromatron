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

#ifndef _ESP8266_H
#define _ESP8266_H

#include "target.h"
#include "netmsg.h"
#include "threading.h"
#include "wifi_cmd.h"

#define WIFI_STATE_ERROR        -2
#define WIFI_STATE_BOOT         -1
#define WIFI_STATE_UNKNOWN      0
#define WIFI_STATE_ALIVE        1
#define WIFI_STATE_SHUTDOWN     2

#define WIFI_SIGNAL 			SIGNAL_SYS_3

#define WIFI_AP_MIN_PASS_LEN    8

#define WIFI_LOADER_MAX_TRIES   8
#define WIFI_WATCHDOG_TIMEOUT   8

// maximum power setting the hardware will support.
// technically this is 20.5 dbm.
// however, 20.5 dbm is only spec'd for 802.11b.
// if we use 802.11n, the radio is only spec'd for 17.5 dbm.
// since we are using an integer, we will set to 17 for our max.
#define WIFI_MAX_HW_TX_POWER    17

void wifi_v_init( void );
void wifi_v_shutdown( void );
bool wifi_b_connected( void );
int8_t wifi_i8_rssi( void );
void wifi_v_get_ssid( char ssid[WIFI_SSID_LEN] );
bool wifi_b_ap_mode( void );
bool wifi_b_ap_mode_enabled( void );
bool wifi_b_attached( void );
bool wifi_b_shutdown( void );
int8_t wifi_i8_get_status( void );
uint32_t wifi_u32_get_received( void );

int8_t wifi_i8_send_udp( netmsg_t netmsg );

int8_t wifi_i8_send_msg( uint8_t data_id, uint8_t *data, uint16_t len );
int8_t wifi_i8_receive_msg( uint8_t data_id, uint8_t *data, uint16_t max_len, uint16_t *bytes_read );

extern int8_t wifi_i8_msg_handler( uint8_t data_id, uint8_t *data, uint16_t len ) __attribute__((weak));

#endif
