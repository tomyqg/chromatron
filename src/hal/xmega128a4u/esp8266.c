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


/*

ESP8266 boot info:

rst cause:
0: undefined
1: power reboot
2: external reset or wake from deep sleep
4: hardware wdt reset


Timing notes:

At UART 4 Mhz, a 128 byte packet transfers in 320 uS.
128 bytes is the size of the ESP8266 hardware FIFO.

theoretical fastest speed for a 576 byte packet is 1.44 ms.

*/

#include "system.h"

#include "threading.h"
#include "timers.h"
#include "os_irq.h"
#include "keyvalue.h"
#include "hal_usart.h"
#include "hal_usb.h"
#include "esp8266.h"
#include "wifi_cmd.h"
#include "netmsg.h"
#include "ip.h"
#include "config.h"
#include "crc.h"
#include "sockets.h"
#include "fs.h"
#include "watchdog.h"
#include "hal_status_led.h"
#include "hal_dma.h"
#include "hash.h"

// #define NO_LOGGING
#include "logging.h"

#define NO_EVENT_LOGGING
#include "event_log.h"

#include "esp_stub.txt"

// these bits in USART.CTRLC seem to be missing from the IO header
#define UDORD 2
#define UCPHA 1



#define WIFI_USART_TIMEOUT 20000
#define WIFI_CONNECT_TIMEOUT 10000

#define WIFI_UART_RX_BUF_SIZE   WIFI_MAIN_BUF_LEN

static uint8_t rx_buf[WIFI_UART_RX_BUF_SIZE];


static uint16_t ports[WIFI_MAX_PORTS];
static bool run_manager;

#define WIFI_RESET_DELAY_MS     20

static volatile bool wifi_rx_ready;

static int8_t wifi_status;
static uint8_t wifi_mac[6];
static uint8_t wifi_status_reg;
static bool connected;
static int8_t wifi_rssi;
static uint32_t wifi_uptime;
static bool default_ap_mode;

static uint16_t wifi_comm_errors;
static uint8_t wifi_connects;

static uint32_t wifi_echo_sent;
static uint32_t wifi_echo_recv;

static uint16_t wifi_rx_udp_fifo_overruns;
static uint16_t wifi_rx_udp_port_overruns;
static uint32_t wifi_udp_received;
static uint32_t wifi_udp_sent;
static uint16_t mem_heap_peak;
static uint16_t intf_max_time;
static uint16_t vm_max_time;
static uint16_t wifi_max_time;
static uint16_t mem_max_time;


static uint16_t wifi_version;

static uint8_t next_msg_id;

static netmsg_t rx_netmsg;
static uint16_t rx_netmsg_index;
static uint16_t rx_netmsg_crc;

static uint8_t router;

// static mem_handle_t wifi_networks_handle = -1;


KV_SECTION_META kv_meta_t wifi_cfg_kv[] = {
    { SAPPHIRE_TYPE_STRING32,      0, 0,                          0,                  cfg_i8_kv_handler,   "wifi_ssid" },
    { SAPPHIRE_TYPE_STRING32,      0, 0,                          0,                  cfg_i8_kv_handler,   "wifi_password" },
    { SAPPHIRE_TYPE_STRING32,      0, KV_FLAGS_PERSIST,           0,                  0,                   "wifi_ssid2" },
    { SAPPHIRE_TYPE_STRING32,      0, KV_FLAGS_PERSIST,           0,                  0,                   "wifi_password2" },
    { SAPPHIRE_TYPE_STRING32,      0, KV_FLAGS_PERSIST,           0,                  0,                   "wifi_ssid3" },
    { SAPPHIRE_TYPE_STRING32,      0, KV_FLAGS_PERSIST,           0,                  0,                   "wifi_password3" },
    { SAPPHIRE_TYPE_STRING32,      0, KV_FLAGS_PERSIST,           0,                  0,                   "wifi_ssid4" },
    { SAPPHIRE_TYPE_STRING32,      0, KV_FLAGS_PERSIST,           0,                  0,                   "wifi_password4" },
    { SAPPHIRE_TYPE_UINT8,         0, KV_FLAGS_PERSIST,           &router,            0,                   "wifi_router" },
    { SAPPHIRE_TYPE_BOOL,          0, 0,                          0,                  cfg_i8_kv_handler,   "wifi_enable_ap" },    
    { SAPPHIRE_TYPE_STRING32,      0, 0,                          0,                  cfg_i8_kv_handler,   "wifi_ap_ssid" },
    { SAPPHIRE_TYPE_STRING32,      0, 0,                          0,                  cfg_i8_kv_handler,   "wifi_ap_password" },
    { SAPPHIRE_TYPE_KEY128,        0, 0,                          0,                  cfg_i8_kv_handler,   "wifi_md5" },
    { SAPPHIRE_TYPE_UINT32,        0, 0,                          0,                  cfg_i8_kv_handler,   "wifi_fw_len" },
};

KV_SECTION_META kv_meta_t wifi_info_kv[] = {
    { SAPPHIRE_TYPE_INT8,          0, 0, &wifi_status,                      0,   "wifi_status" },
    { SAPPHIRE_TYPE_MAC48,         0, 0, &wifi_mac,                         0,   "wifi_mac" },
    { SAPPHIRE_TYPE_INT8,          0, 0, &wifi_rssi,                        0,   "wifi_rssi" },
    { SAPPHIRE_TYPE_UINT32,        0, 0, &wifi_uptime,                      0,   "wifi_uptime" },
    { SAPPHIRE_TYPE_UINT8,         0, 0, &wifi_connects,                    0,   "wifi_connects" },

    { SAPPHIRE_TYPE_UINT16,        0, 0, &wifi_version,                     0,   "wifi_version" },

    { SAPPHIRE_TYPE_UINT16,        0, 0, &wifi_comm_errors,                 0,   "wifi_comm_errors" },
    { SAPPHIRE_TYPE_UINT16,        0, 0, &wifi_rx_udp_fifo_overruns,        0,   "wifi_comm_rx_udp_fifo_overruns" },
    { SAPPHIRE_TYPE_UINT32,        0, 0, &wifi_udp_received,                0,   "wifi_udp_received" },
    { SAPPHIRE_TYPE_UINT32,        0, 0, &wifi_udp_sent,                    0,   "wifi_udp_sent" },

    { SAPPHIRE_TYPE_UINT32,        0, 0, &wifi_echo_sent,                   0,   "wifi_echo_sent" },
    { SAPPHIRE_TYPE_UINT32,        0, 0, &wifi_echo_recv,                   0,   "wifi_echo_recv" },

    { SAPPHIRE_TYPE_UINT16,        0, 0, &wifi_rx_udp_port_overruns,        0,   "wifi_comm_rx_udp_port_overruns" },

    { SAPPHIRE_TYPE_UINT16,        0, 0, &mem_heap_peak,                    0,   "wifi_mem_heap_peak" },

    { SAPPHIRE_TYPE_UINT16,        0, 0, &intf_max_time,                    0,   "wifi_proc_intf_max_time" },
    { SAPPHIRE_TYPE_UINT16,        0, 0, &vm_max_time,                      0,   "wifi_proc_vm_max_time" },
    { SAPPHIRE_TYPE_UINT16,        0, 0, &wifi_max_time,                    0,   "wifi_proc_wifi_max_time" },
    { SAPPHIRE_TYPE_UINT16,        0, 0, &mem_max_time,                     0,   "wifi_proc_mem_max_time" },
};


static void _wifi_v_usart_send_char( uint8_t b ){

    usart_v_send_byte( &WIFI_USART, b );
}

static void _wifi_v_usart_send_data( uint8_t *data, uint16_t len ){

    usart_v_send_data( &WIFI_USART, data, len );
}

static int16_t _wifi_i16_usart_get_char( void ){

    return usart_i16_get_byte( &WIFI_USART );
}

static void _wifi_v_usart_flush( void ){

    SAFE_BUSY_WAIT( _wifi_i16_usart_get_char() >= 0 );
}

#include "io.h"

// void strobe_ss( void ){

//     // WIFI_SS_PORT.DIRSET                 = ( 1 << WIFI_SS_PIN );
//     // WIFI_SS_PORT.OUTSET                 = ( 1 << WIFI_SS_PIN );
//     // _delay_us( 1 );
//     // WIFI_SS_PORT.OUTCLR                 = ( 1 << WIFI_SS_PIN );

//     IO_PIN4_PORT.DIRSET                 = ( 1 << IO_PIN4_PIN );
//     IO_PIN4_PORT.OUTSET                 = ( 1 << IO_PIN4_PIN );
//     _delay_us( 1 );
//     IO_PIN4_PORT.OUTCLR                 = ( 1 << IO_PIN4_PIN );
// }

static uint16_t dma_rx_bytes( void ){

    uint16_t len;

    ATOMIC;
    if( ( DMA.INTFLAGS & WIFI_DMA_CHTRNIF ) != 0 ){

        len = sizeof(rx_buf);
    }
    else{

        len = sizeof(rx_buf) - DMA.WIFI_DMA_CH.TRFCNT;
    }
    END_ATOMIC;

    return len;
}

static void disable_rx_dma( void ){

    ATOMIC;
    DMA.WIFI_DMA_CH.CTRLA &= ~DMA_CH_ENABLE_bm;
    DMA.WIFI_DMA_CH.TRFCNT = 0;
    END_ATOMIC;
}

static void enable_rx_dma( void ){

    ATOMIC;

    disable_rx_dma();

    // flush buffer
    _wifi_v_usart_flush();

    DMA.INTFLAGS = WIFI_DMA_CHTRNIF | WIFI_DMA_CHERRIF; // clear transaction complete interrupt

    DMA.WIFI_DMA_CH.CTRLA = DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_1BYTE_gc;
    DMA.WIFI_DMA_CH.REPCNT = 0;
    DMA.WIFI_DMA_CH.ADDRCTRL = DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_SRCDIR_FIXED_gc | DMA_CH_DESTRELOAD_NONE_gc | DMA_CH_DESTDIR_INC_gc;
    DMA.WIFI_DMA_CH.TRIGSRC = WIFI_USART_DMA_TRIG;
    DMA.WIFI_DMA_CH.TRFCNT = sizeof(rx_buf);

    DMA.WIFI_DMA_CH.SRCADDR0 = ( ( (uint16_t)&WIFI_USART.DATA ) >> 0 ) & 0xFF;
    DMA.WIFI_DMA_CH.SRCADDR1 = ( ( (uint16_t)&WIFI_USART.DATA ) >> 8 ) & 0xFF;
    DMA.WIFI_DMA_CH.SRCADDR2 = 0;

    DMA.WIFI_DMA_CH.DESTADDR0 = ( ( (uint16_t)rx_buf ) >> 0 ) & 0xFF;
    DMA.WIFI_DMA_CH.DESTADDR1 = ( ( (uint16_t)rx_buf ) >> 8 ) & 0xFF;
    DMA.WIFI_DMA_CH.DESTADDR2 = 0;

    DMA.WIFI_DMA_CH.CTRLA |= DMA_CH_ENABLE_bm;
    END_ATOMIC;
}


static void disable_irq( void ){

    // disable port interrupt
    // leave edge detection and pin int mask alone,
    // so we can still get edge detection.


    WIFI_BOOT_PORT.INTCTRL &= ~PORT_INT0LVL_HI_gc;

    // clear the int flag though
    WIFI_BOOT_PORT.INTFLAGS = PORT_INT0IF_bm;
}

static void enable_irq( void ){

    // clear flag
    WIFI_BOOT_PORT.INTFLAGS = PORT_INT0IF_bm;

    // configure boot pin to interrupt, falling edge triggered
    WIFI_BOOT_PORT.INT0MASK |= ( 1 << WIFI_BOOT_PIN );
    WIFI_BOOT_PORT.WIFI_BOOT_PINCTRL &= ~PORT_ISC_LEVEL_gc;
    WIFI_BOOT_PORT.WIFI_BOOT_PINCTRL |= PORT_ISC_FALLING_gc;

    // enable port interrupt
    WIFI_BOOT_PORT.INTCTRL = PORT_INT0LVL_HI_gc;
}



// reset:
// PD transition low to high

// boot mode:
// low = ROM bootloader
// high = normal execution

static void _wifi_v_enter_boot_mode( void ){

    // set up IO
    WIFI_PD_PORT.DIRSET = ( 1 << WIFI_PD_PIN );
    WIFI_PD_PORT.OUTCLR = ( 1 << WIFI_PD_PIN ); // hold chip in reset

    _delay_ms(WIFI_RESET_DELAY_MS);

    WIFI_USART_XCK_PORT.DIRCLR          = ( 1 << WIFI_USART_XCK_PIN );

    WIFI_BOOT_PORT.DIRSET               = ( 1 << WIFI_BOOT_PIN );
    WIFI_BOOT_PORT.WIFI_BOOT_PINCTRL    = 0;
    WIFI_BOOT_PORT.OUTCLR               = ( 1 << WIFI_BOOT_PIN );

    WIFI_SS_PORT.DIRSET                 = ( 1 << WIFI_SS_PIN );
    WIFI_SS_PORT.WIFI_SS_PINCTRL        = 0;
    WIFI_SS_PORT.OUTCLR                 = ( 1 << WIFI_SS_PIN );

    disable_irq();

    // re-init uart
    WIFI_USART_TXD_PORT.DIRSET = ( 1 << WIFI_USART_TXD_PIN );
    WIFI_USART_RXD_PORT.DIRCLR = ( 1 << WIFI_USART_RXD_PIN );
    usart_v_init( &WIFI_USART );

    disable_rx_dma();

    usart_v_set_double_speed( &WIFI_USART, FALSE );
    usart_v_set_baud( &WIFI_USART, BAUD_115200 );
    _wifi_v_usart_flush();

    wifi_status_reg = 0;

    _delay_ms(WIFI_RESET_DELAY_MS);

    // release reset
    WIFI_PD_PORT.OUTSET = ( 1 << WIFI_PD_PIN );

    _delay_ms(WIFI_RESET_DELAY_MS);
    _delay_ms(WIFI_RESET_DELAY_MS);
    _delay_ms(WIFI_RESET_DELAY_MS);

    // return to inputs
    WIFI_BOOT_PORT.DIRCLR               = ( 1 << WIFI_BOOT_PIN );
    WIFI_SS_PORT.DIRCLR                 = ( 1 << WIFI_SS_PIN );
}

static void _wifi_v_enter_normal_mode( void ){

    disable_irq();

    // set up IO
    WIFI_BOOT_PORT.DIRCLR = ( 1 << WIFI_BOOT_PIN );
    WIFI_PD_PORT.DIRSET = ( 1 << WIFI_PD_PIN );
    WIFI_PD_PORT.OUTCLR = ( 1 << WIFI_PD_PIN ); // hold chip in reset

    _delay_ms(WIFI_RESET_DELAY_MS);

    WIFI_BOOT_PORT.WIFI_BOOT_PINCTRL    = 0;

    WIFI_SS_PORT.DIRSET                 = ( 1 << WIFI_SS_PIN );
    WIFI_SS_PORT.WIFI_SS_PINCTRL        = 0;
    WIFI_SS_PORT.OUTCLR                 = ( 1 << WIFI_SS_PIN );

    // disable receive interrupt
    WIFI_USART.CTRLA &= ~USART_RXCINTLVL_HI_gc;

    wifi_status_reg = 0;


    // set XCK pin to output
    WIFI_USART_XCK_PORT.DIRSET = ( 1 << WIFI_USART_XCK_PIN );
    WIFI_USART_XCK_PORT.OUTSET = ( 1 << WIFI_USART_XCK_PIN );

    // set boot pin to high
    WIFI_BOOT_PORT.WIFI_BOOT_PINCTRL    = PORT_OPC_PULLUP_gc;

    _delay_ms(WIFI_RESET_DELAY_MS);

    // release reset
    WIFI_PD_PORT.OUTSET = ( 1 << WIFI_PD_PIN );

    _delay_ms(WIFI_RESET_DELAY_MS);

    disable_rx_dma();

    // re-init uart
    WIFI_USART_TXD_PORT.DIRSET = ( 1 << WIFI_USART_TXD_PIN );
    WIFI_USART_RXD_PORT.DIRCLR = ( 1 << WIFI_USART_RXD_PIN );
    usart_v_init( &WIFI_USART );
    usart_v_set_double_speed( &WIFI_USART, TRUE );
    usart_v_set_baud( &WIFI_USART, BAUD_2000000 );

    wifi_status_reg = 0;
}


static void wifi_v_reset_rx_buffer( void ){

    disable_rx_dma();

    rx_buf[0] = WIFI_COMM_IDLE;

    // set up DMA
    enable_rx_dma();
}

static void wifi_v_reset_comm( void ){

    wifi_v_reset_rx_buffer();
    _wifi_v_usart_send_char( WIFI_COMM_RESET );   
}

static void wifi_v_set_rx_ready( void ){
        
    wifi_v_reset_rx_buffer();

    WIFI_USART_XCK_PORT.OUTCLR = ( 1 << WIFI_USART_XCK_PIN );
    _delay_us( 10 );
    WIFI_USART_XCK_PORT.OUTSET = ( 1 << WIFI_USART_XCK_PIN );
}

static uint8_t wifi_u8_get_control_byte( void ){

    return rx_buf[0];
}

static int8_t wifi_i8_rx_data_received( void ){

    if( wifi_u8_get_control_byte() == WIFI_COMM_IDLE ){

        return -1;
    }

    wifi_data_header_t *header = (wifi_data_header_t *)&rx_buf[1];

    if( dma_rx_bytes() >= ( 1 + sizeof(wifi_data_header_t) + header->len ) ){

        return 0;
    }

    return -2;
}

bool wifi_b_comm_ready( void ){

    ATOMIC;
    bool temp = wifi_rx_ready;
    END_ATOMIC;

    return temp;
}

// waits up to WIFI_USART_TIMEOUT microseconds for comm to be ready
bool wifi_b_wait_comm_ready( void ){
    
    uint32_t timeout = tmr_u32_get_system_time_us();

    SAFE_BUSY_WAIT( ( tmr_u32_elapsed_time_us( timeout ) < WIFI_USART_TIMEOUT ) && !wifi_b_comm_ready() );

    return wifi_b_comm_ready();
}


int8_t wifi_i8_send_msg( uint8_t data_id, uint8_t *data, uint8_t len ){

    ASSERT( len <= WIFI_MAX_DATA_LEN );
    
    if( !wifi_b_comm_ready() ){

        log_v_debug_P( PSTR("rx not ready! %d"), data_id ); 

        return -1;  
    }

    next_msg_id++;

    uint16_t crc = crc_u16_start();

    wifi_data_header_t header;
    header.msg_id   = next_msg_id;
    header.len      = len;
    header.data_id  = data_id;
    header.crc      = 0;
    crc = crc_u16_partial_block( crc, (uint8_t *)&header, sizeof(header) );    

    crc = crc_u16_partial_block( crc, data, len );

    header.crc = crc_u16_finish( crc );


    ATOMIC;
    wifi_rx_ready = FALSE;
    END_ATOMIC;

    _wifi_v_usart_send_char( WIFI_COMM_DATA );
    _wifi_v_usart_send_data( (uint8_t *)&header, sizeof(header) );
    _wifi_v_usart_send_data( data, len );

    return 0;
}

int8_t wifi_i8_send_msg_blocking( uint8_t data_id, uint8_t *data, uint8_t len ){

    if( !wifi_b_wait_comm_ready() ){

        return -1;
    }

    if( wifi_i8_send_msg( data_id, data, len ) < 0 ){

        return -2;
    }

    return 0;
}

int8_t wifi_i8_send_msg_response( 
    uint8_t data_id, 
    uint8_t *data, 
    uint8_t len,
    uint8_t *response,
    uint8_t response_len ){

    int8_t status = 0;

    uint32_t timeout = tmr_u32_get_system_time_us();

    SAFE_BUSY_WAIT( ( tmr_u32_elapsed_time_us( timeout ) < WIFI_USART_TIMEOUT ) && !wifi_b_comm_ready() );

    if( wifi_i8_send_msg( data_id, data, len ) < 0 ){

        status = -1;
        goto end;
    }

    SAFE_BUSY_WAIT( ( tmr_u32_elapsed_time_us( timeout ) < WIFI_USART_TIMEOUT ) && ( wifi_i8_rx_data_received() < 0 ) );

    if( wifi_i8_rx_data_received() < 0 ){

        status = -2;
        goto end;
    }


    wifi_data_header_t *header = (wifi_data_header_t *)&rx_buf[1];
    uint8_t *response_data = (uint8_t *)( header + 1 );

    if( crc_u16_block( response_data, header->len ) == header->crc ){

        // bounds check
        if( header->len < response_len ){

            response_len = header->len;
        }

        memcpy( response, response_data, response_len );
    }
    else{

        log_v_debug_P( PSTR("Wifi crc error") );
        status = -3;
    }

end:
    wifi_v_set_rx_ready();

    return status;
}

void open_close_port( uint8_t protocol, uint16_t port, bool open ){

    if( protocol == IP_PROTO_UDP ){

        if( open ){

            // search for duplicate
            for( uint8_t i = 0; i < cnt_of_array(ports); i++ ){

                if( ports[i] == port ){

                    return;
                }
            }

            // search for empty slot
            for( uint8_t i = 0; i < cnt_of_array(ports); i++ ){

                if( ports[i] == 0 ){

                    ports[i] = port;

                    run_manager = TRUE;

                    break;
                }
            }
        }
        else{

            // search for port
            for( uint8_t i = 0; i < cnt_of_array(ports); i++ ){

                if( ports[i] == port ){

                    ports[i] = 0;

                    run_manager = TRUE;

                    break;
                }
            }
        }
    }
}

int8_t wifi_i8_send_udp( netmsg_t netmsg ){

    if( !wifi_b_connected() ){

        return NETMSG_TX_ERR_RELEASE;
    }

    int8_t status = -1;

    netmsg_state_t *netmsg_state = netmsg_vp_get_state( netmsg );

    ASSERT( netmsg_state->type == NETMSG_TYPE_UDP );

    uint16_t data_len = 0;

    uint16_t crc = crc_u16_start();

    uint8_t *data = 0;
    uint8_t *h2 = 0;
    uint16_t h2_len = 0;

    if( netmsg_state->data_handle > 0 ){

        data = mem2_vp_get_ptr( netmsg_state->data_handle );
        data_len = mem2_u16_get_size( netmsg_state->data_handle );
    }

    // header 2, if present
    if( netmsg_state->header_2_handle > 0 ){

        h2 = mem2_vp_get_ptr( netmsg_state->header_2_handle );
        h2_len = mem2_u16_get_size( netmsg_state->header_2_handle );

        crc = crc_u16_partial_block( crc, h2, h2_len );
    }

    if( netmsg_state->data_handle > 0 ){

        crc = crc_u16_partial_block( crc, data, data_len );
    }

    // setup header
    wifi_msg_udp_header_t udp_header;
    udp_header.addr = netmsg_state->raddr.ipaddr;
    udp_header.lport = netmsg_state->laddr.port;
    udp_header.rport = netmsg_state->raddr.port;
    udp_header.len = data_len + h2_len;
    udp_header.crc = crc_u16_finish( crc );

    // send header
    int8_t ret_val = wifi_i8_send_msg_blocking( WIFI_DATA_ID_UDP_HEADER, (uint8_t *)&udp_header, sizeof(udp_header) );

    if( ret_val < 0 ){

        log_v_debug_P( PSTR("error %d"), ret_val );

        status = -5;
        goto error;
    }

    if( h2_len > 0 ){

        if( wifi_i8_send_msg_blocking( WIFI_DATA_ID_UDP_DATA, h2, h2_len ) < 0 ){

            status = -3;
            goto error;
        }
    }

    while( data_len > 0 ){

        uint16_t copy_len = data_len;

        if( copy_len > WIFI_MAX_DATA_LEN ){

            copy_len = WIFI_MAX_DATA_LEN;
        }

        if( wifi_i8_send_msg_blocking( WIFI_DATA_ID_UDP_DATA, data, copy_len ) < 0 ){

            log_v_debug_P( PSTR("port %u -> %u len %d datalen: %d"), udp_header.lport, udp_header.rport, copy_len, data_len );

            status = -4;
            goto error;
        }

        data += copy_len;
        data_len -= copy_len;
    }

    return NETMSG_TX_OK_RELEASE;

error:
    log_v_debug_P( PSTR("send udp error %d"), status );
    
    return NETMSG_TX_ERR_RELEASE;
}


#define SLIP_END        0xC0
#define SLIP_ESC        0xDB
#define SLIP_ESC_END    0xDC
#define SLIP_ESC_ESC    0xDD

#define ESP_FLASH_BEGIN 0x02
#define ESP_FLASH_DATA  0x03
#define ESP_FLASH_END   0x04
#define ESP_MEM_BEGIN   0x05
#define ESP_MEM_END     0x06
#define ESP_MEM_DATA    0x07
#define ESP_SYNC        0x08
#define ESP_WRITE_REG   0x09
#define ESP_READ_REG    0x0a

// Maximum block sized for RAM and Flash writes, respectively.
#define ESP_RAM_BLOCK   0x1800
#define ESP_FLASH_BLOCK 0x400

// Default baudrate. The ROM auto-bauds, so we can use more or less whatever we want.
#define ESP_ROM_BAUD    115200

// First byte of the application image
#define ESP_IMAGE_MAGIC 0xe9

// Initial state for the checksum routine
#define ESP_CHECKSUM_MAGIC 0xef

// OTP ROM addresses
#define ESP_OTP_MAC0    0x3ff00050
#define ESP_OTP_MAC1    0x3ff00054
#define ESP_OTP_MAC3    0x3ff0005c

// Flash sector size, minimum unit of erase.
#define ESP_FLASH_SECTOR 0x1000

#define ESP_CESANTA_BAUD                2000000
#define ESP_CESANTA_BAUD_USART_SETTING  BAUD_2000000

#define ESP_CESANTA_FLASH_SECTOR_SIZE   4096

#define ESP_CESANTA_CMD_FLASH_ERASE     0
#define ESP_CESANTA_CMD_FLASH_WRITE     1
#define ESP_CESANTA_CMD_FLASH_READ      2
#define ESP_CESANTA_CMD_FLASH_DIGEST    3

#define MD5_LEN 16

void slip_v_send_byte( uint8_t b ){

    if( b == SLIP_END ){

        _wifi_v_usart_send_char( SLIP_ESC );
        _wifi_v_usart_send_char( SLIP_ESC_END );
    }
    else if( b == SLIP_ESC ){

        _wifi_v_usart_send_char( SLIP_ESC );
        _wifi_v_usart_send_char( SLIP_ESC_ESC );
    }
    else{

        _wifi_v_usart_send_char( b );
    }
}

void slip_v_send_data( uint8_t *data, uint16_t len ){

    while( len > 0 ){

        slip_v_send_byte( *data );

        data++;
        len--;
    }
}

void esp_v_send_header( uint8_t op, uint16_t len, uint32_t checksum ){

    _wifi_v_usart_flush();

    _wifi_v_usart_send_char( SLIP_END );

    slip_v_send_byte( 0x00 );
    slip_v_send_byte( op );
    slip_v_send_byte( len & 0xff );
    slip_v_send_byte( len >> 8 );
    slip_v_send_byte( checksum & 0xff );
    slip_v_send_byte( checksum >> 8 );
    slip_v_send_byte( checksum >> 16 );
    slip_v_send_byte( checksum >> 24 );
}


void esp_v_command( uint8_t op, uint8_t *data, uint16_t len, uint32_t checksum ){

    _wifi_v_usart_flush();

    esp_v_send_header( op, len, checksum );

    slip_v_send_data( data, len );

    _wifi_v_usart_send_char( SLIP_END );
}

static const PROGMEM uint8_t sync_data[] = {
    0x07,
    0x07,
    0x12,
    0x20,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
    0x55,
};

void esp_v_send_sync( void ){

    uint8_t buf[sizeof(sync_data)];
    memcpy_P( buf, sync_data, sizeof(buf) );

    esp_v_command( ESP_SYNC, buf, sizeof(buf), 0 );
}

#define ESP_SYNC_TIMEOUT 5000
#define ESP_CMD_TIMEOUT 50000
#define ESP_CESANTA_TIMEOUT 50000
#define ESP_ERASE_TIMEOUT 5000000

typedef struct{
    uint8_t resp;
    uint8_t opcode;
    uint16_t len;
    uint32_t val;
} esp_response_t;

int8_t esp_i8_wait_response( uint8_t *buf, uint8_t len, uint32_t timeout ){

    int8_t status = -100;
    uint32_t start_time = tmr_u32_get_system_time_us();

    uint8_t next_byte = 0;
    memset( rx_buf, 0xff, sizeof(rx_buf) );
    enable_rx_dma();

    // waiting for frame start
    while( rx_buf[next_byte] != SLIP_END ){

        if( tmr_u32_elapsed_time_us( start_time ) > timeout ){

            status = -1;
            goto end;
        }
    }

    next_byte++;

    while(1){

        if( len == 0 ){

            status = -3;
            goto end;
        }

        // wait for byte
        while( dma_rx_bytes() == next_byte ){

            if( tmr_u32_elapsed_time_us( start_time ) > timeout ){

                status = -4;
                goto end;
            }
        }

        uint8_t b = rx_buf[next_byte];
        next_byte++;

        if( b == SLIP_END ){

            status = 0;
            goto end;
        }
        else if( b == SLIP_ESC ){

            // wait for byte
            while( dma_rx_bytes() == next_byte ){

                if( tmr_u32_elapsed_time_us( start_time ) > timeout ){

                    status = -5;
                    goto end;
                }
            }

            b = rx_buf[next_byte];
            next_byte++;

            if( b == SLIP_ESC_END ){

                *buf = SLIP_END;
                buf++;
                len--;
            }
            else if( b == SLIP_ESC_ESC ){

                *buf = SLIP_ESC;
                buf++;
                len--;
            }
            else{

                status = -6;
                goto end;
            }
        }
        else{

            *buf = b;
            buf++;
            len--;
        }
    }

end:

    if( status != 0 ){

        log_v_debug_P( PSTR("loader error: %d"), status );
        log_v_debug_P( PSTR("%2x %2x %2x %2x %2x %2x %2x %2x"), rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4], rx_buf[5], rx_buf[6], rx_buf[7] );
    }

    disable_rx_dma();

    return status;
}

typedef struct{
    uint32_t size;
    uint32_t num_blocks;
    uint32_t block_size;
    uint32_t offset;
} esp_mem_begin_t;

typedef struct{
    uint32_t size;
    uint32_t seq;
    uint32_t reserved0;
    uint32_t reserved1;
} esp_mem_block_t;

typedef struct{
    uint32_t zero;
    uint32_t entrypoint;
} esp_mem_finish_t;

int8_t esp_i8_load_cesanta_stub( void ){

    uint8_t buf[32];
    memset( buf, 0xff, sizeof(buf) );
    esp_response_t *resp = (esp_response_t *)buf;
    uint8_t *resp_body = (uint8_t *)( resp + 1 );

    uint32_t param = ESP_CESANTA_BAUD;

    #define STUB_CODE_SIZE ( sizeof(esp_stub_code) + sizeof(param) )

    esp_mem_begin_t mem_begin;
    mem_begin.size = STUB_CODE_SIZE;
    mem_begin.num_blocks = 1;
    mem_begin.block_size = STUB_CODE_SIZE;
    mem_begin.offset = ESP_STUB_PARAMS_START;

    esp_v_command( ESP_MEM_BEGIN, (uint8_t *)&mem_begin, sizeof(mem_begin), 0 );

    if( esp_i8_wait_response( buf, sizeof(buf), ESP_CMD_TIMEOUT ) < 0 ){

        return -1;
    }

    if( ( resp_body[0] != 0 ) || ( resp_body[1] != 0 ) ){

        log_v_debug_P( PSTR("resp: %x %x %x %lx %x %x"),
                resp->resp,
                resp->opcode,
                resp->len,
                resp->val,
                resp_body[0],
                resp_body[1]
                );

        return -2;
    }

    uint8_t checksum = ESP_CHECKSUM_MAGIC;

    // check sum over param
    checksum ^= ( param >> 24 ) & 0xff;
    checksum ^= ( param >> 16 ) & 0xff;
    checksum ^= ( param >> 8 )  & 0xff;
    checksum ^= ( param >> 0 )  & 0xff;

    for( uint16_t i = 0; i < sizeof(esp_stub_code); i++ ){

        checksum ^= pgm_read_byte( &esp_stub_code[i] );
    }

    esp_v_send_header( ESP_MEM_DATA, sizeof(esp_mem_block_t) + STUB_CODE_SIZE, checksum );

    esp_mem_block_t mem_block;
    mem_block.size = STUB_CODE_SIZE;
    mem_block.seq = 0;
    mem_block.reserved0 = 0;
    mem_block.reserved1 = 0;

    slip_v_send_data( (uint8_t *)&mem_block, sizeof(mem_block) );
    slip_v_send_data( (uint8_t *)&param, sizeof(param) );

    for( uint16_t i = 0; i < sizeof(esp_stub_code); i++ ){

        uint8_t b = pgm_read_byte( &esp_stub_code[i] );

        slip_v_send_byte( b );
    }

    _wifi_v_usart_send_char( SLIP_END );

    if( esp_i8_wait_response( buf, sizeof(buf), ESP_CMD_TIMEOUT ) < 0 ){

        return -3;
    }

    if( ( resp_body[0] != 0 ) || ( resp_body[1] != 0 ) ){

        return -4;
    }

    uint8_t stub_data[sizeof(esp_stub_data)];
    memcpy_P( stub_data, esp_stub_data, sizeof(stub_data) );
    mem_begin.size = sizeof(stub_data);
    mem_begin.num_blocks = 1;
    mem_begin.block_size = sizeof(stub_data);
    mem_begin.offset = ESP_STUB_DATA_START;

    esp_v_command( ESP_MEM_BEGIN, (uint8_t *)&mem_begin, sizeof(mem_begin), 0 );

    if( esp_i8_wait_response( buf, sizeof(buf), ESP_CMD_TIMEOUT ) < 0 ){

        return -5;
    }

    if( ( resp_body[0] != 0 ) || ( resp_body[1] != 0 ) ){

        return -6;
    }

    checksum = ESP_CHECKSUM_MAGIC;

    for( uint16_t i = 0; i < sizeof(stub_data); i++ ){

        checksum ^= stub_data[i];
    }

    mem_block.size = sizeof(stub_data);

    esp_v_send_header( ESP_MEM_DATA, sizeof(esp_mem_block_t) + sizeof(stub_data), checksum );
    slip_v_send_data( (uint8_t *)&mem_block, sizeof(mem_block) );
    slip_v_send_data( stub_data, sizeof(stub_data) );

    _wifi_v_usart_send_char( SLIP_END );

    if( esp_i8_wait_response( buf, sizeof(buf), ESP_CMD_TIMEOUT ) < 0 ){

        return -7;
    }

    if( ( resp_body[0] != 0 ) || ( resp_body[1] != 0 ) ){

        return -8;
    }

    esp_mem_finish_t mem_finish;
    mem_finish.zero = 0;
    mem_finish.entrypoint = ESP_STUB_ENTRY;

    esp_v_command( ESP_MEM_END, (uint8_t *)&mem_finish, sizeof(mem_finish), 0 );

    if( esp_i8_wait_response( buf, sizeof(buf), ESP_CMD_TIMEOUT ) < 0 ){

        return -9;
    }

    if( ( resp_body[0] != 0 ) || ( resp_body[1] != 0 ) ){

        return -10;
    }

    return 0;
}


// Cesanta protocol
typedef struct{
    uint32_t addr;
    uint32_t len;
    uint32_t erase;
} esp_write_flash_t;

int8_t esp_i8_load_flash( file_t file ){

    // make sure file is at position 0!
    fs_v_seek( file, 0 );

    int32_t file_len = fs_i32_get_size( file );

    // file image will have md5 checksum appended, so throw away last 16 bytes
    file_len -= MD5_LEN;

    if( file_len < 0 ){

        return -1;
    }

    uint32_t cfg_file_len;
    cfg_i8_get( CFG_PARAM_WIFI_FW_LEN, &cfg_file_len );

    // check for config and file length mismatch
    if( cfg_file_len != (uint32_t)file_len ){

        return -1;
    }

    memset( (uint8_t *)rx_buf, 0xff, sizeof(rx_buf) );
    enable_rx_dma();

    // cast rx_buf to a volatile pointer.
    // otherwise, GCC will attempt to be clever and cache
    // the first byte of rx_buf, which will cause
    // the check for SLIP_END to fail.
    volatile uint8_t *buf = rx_buf;

    esp_write_flash_t cmd;
    cmd.addr = 0;
    cmd.len = file_len;
    cmd.erase = 1; // erase before write

    _wifi_v_usart_send_char( SLIP_END );
    slip_v_send_byte( ESP_CESANTA_CMD_FLASH_WRITE );
    _wifi_v_usart_send_char( SLIP_END );

    _wifi_v_usart_send_char( SLIP_END );
    slip_v_send_data( (uint8_t *)&cmd, sizeof(cmd) );
    _wifi_v_usart_send_char( SLIP_END );

    for( uint8_t i = 0; i < 100; i++ ){

        _delay_us( 50 );

        if( buf[5] == SLIP_END ){

            break;
        }
    }

    if( !( ( buf[0] == SLIP_END ) &&
           ( buf[1] == 0 ) &&
           ( buf[2] == 0 ) &&
           ( buf[3] == 0 ) &&
           ( buf[4] == 0 ) &&
           ( buf[5] == SLIP_END ) ) ){

        log_v_debug_P( PSTR("error") );

        disable_rx_dma();
        return -1;
    }

    // This buffer eats a lot of stack.
    // At some point we could rework this to use the rx_buf,
    // or load the data in smaller chunks.
    uint8_t file_buf[256];
    int32_t len = 0;

    while( len < file_len ){

        wdg_v_reset();

        memset( file_buf, 0xff, sizeof(file_buf) );

        int16_t read = fs_i16_read( file, file_buf, sizeof(file_buf) );

        if( read < 0 ){

            disable_rx_dma();
            return -2;
        }

        if( ( file_buf[0] == ESP_IMAGE_MAGIC ) && ( len == 0 ) ){

            file_buf[2] = 0;
            file_buf[3] = 0;
        }

        _wifi_v_usart_send_data( file_buf, sizeof(file_buf) );

        len += sizeof(file_buf);

        if( ( len % 1024 ) == 0 ){

            memset( (uint8_t *)rx_buf, 0xff, sizeof(rx_buf) );
            enable_rx_dma();

            for( uint8_t i = 0; i < 250; i++ ){

                _delay_ms( 1 );

                // checking that last response packet got sent,
                // but not actually checking value
                if( buf[0] == SLIP_END ){

                    _delay_ms( 1 );

                    break;
                }
            }
        }
    }

    disable_rx_dma();

    return 0;
}

// Cesanta protocol
typedef struct{
    uint32_t addr;
    uint32_t len;
    uint32_t block_size;
} esp_digest_t;

int8_t esp_i8_md5( uint32_t len, uint8_t digest[MD5_LEN] ){

    memset( rx_buf, 0xff, sizeof(rx_buf) );
    enable_rx_dma();

    esp_digest_t cmd;
    cmd.addr = 0;
    cmd.len = len;
    cmd.block_size = 0;


    _wifi_v_usart_send_char( SLIP_END );
    slip_v_send_byte( ESP_CESANTA_CMD_FLASH_DIGEST );
    _wifi_v_usart_send_char( SLIP_END );

    _wifi_v_usart_send_char( SLIP_END );
    slip_v_send_data( (uint8_t *)&cmd, sizeof(cmd) );
    _wifi_v_usart_send_char( SLIP_END );

    // cast rx_buf to a volatile pointer.
    // otherwise, GCC will attempt to be clever and cache
    // the first byte of rx_buf, which will cause
    // the check for SLIP_END to fail.
    volatile uint8_t *buf = rx_buf;

    for( uint8_t i = 0; i < 250; i++ ){

        _delay_ms( 2 );

        // checking that last response packet got sent,

        if( buf[0] == SLIP_END ){

            _delay_ms( 5 ); // wait for rest of response

            break;
        }
    }

    if( buf[0] != SLIP_END ){

        disable_rx_dma();
        return -2;
    }


    memset( digest, 0xff, MD5_LEN );
    uint8_t md5_idx = 0;

    // parse response
    for( uint8_t i = 1; i < sizeof(rx_buf); i++ ){

        if( md5_idx >= MD5_LEN ){

            break;
        }

        if( rx_buf[i] == SLIP_END ){

            break;
        }
        else if( rx_buf[i] == SLIP_ESC ){

            i++;

            if( rx_buf[i] == SLIP_ESC_END ){

                digest[md5_idx] = SLIP_END;
                md5_idx++;
            }
            else if( rx_buf[i] == SLIP_ESC_ESC ){

                digest[md5_idx] = SLIP_ESC;
                md5_idx++;
            }
        }
        else{

            digest[md5_idx] = rx_buf[i];
            md5_idx++;
        }
    }

    disable_rx_dma();
    return 0;
}

void esp_v_flash_end( void ){

    uint32_t cmd = 1; // do not reboot

    esp_v_command( ESP_FLASH_END, (uint8_t *)&cmd, sizeof(cmd), 0 );
}


typedef struct{
    uint8_t timeout;
    file_t fw_file;
} loader_thread_state_t;

PT_THREAD( wifi_loader_thread( pt_t *pt, loader_thread_state_t *state ) );



PT_THREAD( wifi_connection_manager_thread( pt_t *pt, void *state ) )
{
PT_BEGIN( pt );
    
    // check if we are connected
    while( !wifi_b_connected() ){

        wifi_rssi = -127;
        
        THREAD_WAIT_WHILE( pt, !wifi_b_comm_ready() );

        bool ap_mode = wifi_b_ap_mode_enabled();


        char ssid[WIFI_SSID_LEN];
        cfg_i8_get( CFG_PARAM_WIFI_SSID, ssid );
        
        // check if wifi settings were present
        if( ssid[0] == 0 ){        

            // switch to AP mode
            ap_mode = TRUE;
        }

        // station mode
        if( !ap_mode ){
    
            // log_v_debug_P( PSTR("Start scan") );

            // // run a scan
            // wifi_i8_send_msg( WIFI_DATA_ID_WIFI_SCAN, 0, 0 );

            // thread_v_set_alarm( tmr_u32_get_system_time_ms() + WIFI_CONNECT_TIMEOUT );    
            // THREAD_WAIT_WHILE( pt, ( wifi_networks_handle < 0 ) &&
            //                        ( thread_b_alarm_set() ) );


            // uint32_t network_hashes[4];
            // memset( network_hashes, 0, sizeof(network_hashes) );
            // int8_t selected_network = -1;

            // if( wifi_networks_handle > 0 ){

            //     // gather available networks
            //     char ssid[WIFI_SSID_LEN];

            //     memset( ssid, 0, sizeof(ssid) );
            //     cfg_i8_get( CFG_PARAM_WIFI_SSID, ssid );
            //     network_hashes[0] = hash_u32_string( ssid );

            //     memset( ssid, 0, sizeof(ssid) );
            //     kv_i8_get_by_hash( __KV__wifi_ssid2, ssid, sizeof(ssid) );
            //     network_hashes[1] = hash_u32_string( ssid );

            //     memset( ssid, 0, sizeof(ssid) );
            //     kv_i8_get_by_hash( __KV__wifi_ssid3, ssid, sizeof(ssid) );
            //     network_hashes[2] = hash_u32_string( ssid );

            //     memset( ssid, 0, sizeof(ssid) );
            //     kv_i8_get_by_hash( __KV__wifi_ssid4, ssid, sizeof(ssid) );
            //     network_hashes[3] = hash_u32_string( ssid );


            //     wifi_msg_scan_results_t *msg = (wifi_msg_scan_results_t *)mem2_vp_get_ptr( wifi_networks_handle );

            //     // search for matching networks and track best signal
            //     int8_t best_rssi = -120;
            //     int8_t best_index = -1;

            //     for( uint8_t i = 0; i < msg->count; i++ ){

            //         // log_v_debug_P(PSTR("%ld %lu"), msg->networks[i].rssi, msg->networks[i].ssid_hash );

            //         // is this RSSI any good?

            //         if( msg->networks[i].rssi <= best_rssi ){

            //             continue;
            //         }

            //         // do we have this SSID?
            //         for( uint8_t j = 0; j < cnt_of_array(network_hashes); j++ ){

            //             if( network_hashes[j] == msg->networks[i].ssid_hash ){

            //                 // match!

            //                 // record RSSI and index
            //                 best_index = j;
            //                 best_rssi = msg->networks[i].rssi;

            //                 break;
            //             }
            //         }
            //     }

            //     if( best_index >= 0 ){

            //         // log_v_debug_P( PSTR("Best: %d %lu"), best_rssi, network_hashes[best_index] );

            //         selected_network = best_index;
            //     }

            //     mem2_v_free( wifi_networks_handle );
            //     wifi_networks_handle = -1;     
            // }
            
                
            wifi_msg_connect_t msg;
            memset( &msg, 0, sizeof(msg) );

            if( router == 1 ){

                kv_i8_get_by_hash( __KV__wifi_ssid2, msg.ssid, sizeof(msg.ssid) );
                kv_i8_get_by_hash( __KV__wifi_password2, msg.pass, sizeof(msg.pass) );  
            }
            else if( router == 2 ){

                kv_i8_get_by_hash( __KV__wifi_ssid3, msg.ssid, sizeof(msg.ssid) );
                kv_i8_get_by_hash( __KV__wifi_password3, msg.pass, sizeof(msg.pass) );  
            }
            else if( router == 3 ){

                kv_i8_get_by_hash( __KV__wifi_ssid4, msg.ssid, sizeof(msg.ssid) );
                kv_i8_get_by_hash( __KV__wifi_password4, msg.pass, sizeof(msg.pass) );  
            }
            else{

                cfg_i8_get( CFG_PARAM_WIFI_SSID, msg.ssid );
                cfg_i8_get( CFG_PARAM_WIFI_PASSWORD, msg.pass );
            }

            // check if a router was configured
            if( msg.ssid[0] == 0 ){

                router++;

                if( router >= 4 ){

                    router = 0;
                }

                THREAD_YIELD( pt );

                continue;
            }

            log_v_debug_P( PSTR("Connecting to: %s"), msg.ssid );
            wifi_i8_send_msg( WIFI_DATA_ID_CONNECT, (uint8_t *)&msg, sizeof(msg) );

            thread_v_set_alarm( tmr_u32_get_system_time_ms() + WIFI_CONNECT_TIMEOUT );    
            THREAD_WAIT_WHILE( pt, ( !wifi_b_connected() ) &&
                                   ( thread_b_alarm_set() ) );

            // check if connected
            if( !wifi_b_connected() ){

                router++;

                if( router >= 4 ){

                    router = 0;
                }
            }
        }
        // AP mode
        else{

            // wait until we have a MAC address
            THREAD_WAIT_WHILE( pt, wifi_mac[0] == 0 );

            log_v_debug_P( PSTR("starting AP") );

            // AP mode
            wifi_msg_ap_connect_t ap_msg;

            cfg_i8_get( CFG_PARAM_WIFI_AP_SSID, ap_msg.ssid );
            cfg_i8_get( CFG_PARAM_WIFI_AP_PASSWORD, ap_msg.pass );

            // check if AP mode SSID is set:
            if( ap_msg.ssid[0] == 0 ){

                // set up default AP
                memset( ap_msg.ssid, 0, sizeof(ap_msg.ssid) );
                memset( ap_msg.pass, 0, sizeof(ap_msg.pass) );

                strlcpy_P( ap_msg.ssid, PSTR("Chromatron_"), sizeof(ap_msg.ssid) );

                char mac[16];
                memset( mac, 0, sizeof(mac) );
                snprintf_P( &mac[0], 3, PSTR("%02x"), wifi_mac[3] );
                snprintf_P( &mac[2], 3, PSTR("%02x"), wifi_mac[4] ); 
                snprintf_P( &mac[4], 3, PSTR("%02x"), wifi_mac[5] );

                strncat( ap_msg.ssid, mac, sizeof(ap_msg.ssid) );

                strlcpy_P( ap_msg.pass, PSTR("12345678"), sizeof(ap_msg.pass) );

                default_ap_mode = TRUE;
            }
            else if( strnlen( ap_msg.pass, sizeof(ap_msg.pass) ) < WIFI_AP_MIN_PASS_LEN ){

                log_v_warn_P( PSTR("AP mode password must be at least 8 characters.") );

                // disable ap mode
                bool temp = FALSE;
                cfg_v_set( CFG_PARAM_WIFI_ENABLE_AP, &temp );

                goto end;
            }

            log_v_debug_P( PSTR("ssid: %s"), ap_msg.ssid );

            // check if wifi settings were present
            if( ap_msg.ssid[0] != 0 ){        

                wifi_i8_send_msg( WIFI_DATA_ID_AP_MODE, (uint8_t *)&ap_msg, sizeof(ap_msg) );

                thread_v_set_alarm( tmr_u32_get_system_time_ms() + WIFI_CONNECT_TIMEOUT );    
                THREAD_WAIT_WHILE( pt, ( !wifi_b_connected() ) &&
                                       ( thread_b_alarm_set() ) );
            }
        }

end:
        if( !wifi_b_connected() ){

            TMR_WAIT( pt, 500 );
        }
    }     

    while( wifi_b_connected() ){

        if( !connected ){

            if( !wifi_b_ap_mode_enabled() ){

                wifi_connects++;
                connected = TRUE;
                log_v_debug_P( PSTR("Wifi connected") );
            }
        }

        thread_v_set_alarm( tmr_u32_get_system_time_ms() + 2000 );    
        THREAD_WAIT_WHILE( pt, ( run_manager == FALSE ) &&
                               ( thread_b_alarm_set() ) );

        THREAD_WAIT_WHILE( pt, !wifi_b_comm_ready() );
        wifi_i8_send_msg( WIFI_DATA_ID_PORTS, (uint8_t *)&ports, sizeof(ports) );

        run_manager = FALSE;
    }

    connected = FALSE;
    log_v_debug_P( PSTR("Wifi disconnected") );

    THREAD_RESTART( pt );

PT_END( pt );
}


static int8_t process_rx_data( void ){

    if( wifi_i8_rx_data_received() < 0 ){

        return -1;
    }

    
    uint8_t buf[WIFI_UART_RX_BUF_SIZE];
    wifi_data_header_t *header = (wifi_data_header_t *)&rx_buf[1];

    memcpy( buf, &rx_buf[1], sizeof(wifi_data_header_t) + header->len );

    wifi_v_set_rx_ready();

    header = (wifi_data_header_t *)buf;
    uint8_t *data = (uint8_t *)( header + 1 );

    uint16_t msg_crc = header->crc;
    header->crc = 0;


    if( crc_u16_block( (uint8_t *)header, header->len + sizeof(wifi_data_header_t) ) != msg_crc ){

        log_v_debug_P( PSTR("Wifi crc error") );
        return -2;
    }


    if( header->data_id == WIFI_DATA_ID_STATUS ){

        if( header->len != sizeof(wifi_msg_status_t) ){

            goto len_error;
        }

        wifi_msg_status_t *msg = (wifi_msg_status_t *)data;

        wifi_status_reg = msg->flags;
    }  
    else if( header->data_id == WIFI_DATA_ID_INFO ){

        if( header->len != sizeof(wifi_msg_info_t) ){

            goto len_error;
        }

        wifi_msg_info_t *msg = (wifi_msg_info_t *)data;

        wifi_version            = msg->version;
        wifi_rssi               = msg->rssi;
        memcpy( wifi_mac, msg->mac, sizeof(wifi_mac) );

        uint64_t current_device_id = 0;
        cfg_i8_get( CFG_PARAM_DEVICE_ID, &current_device_id );
        uint64_t device_id = 0;
        memcpy( &device_id, wifi_mac, sizeof(wifi_mac) );

        if( current_device_id != device_id ){

            cfg_v_set( CFG_PARAM_DEVICE_ID, &device_id );
        }

        cfg_v_set( CFG_PARAM_IP_ADDRESS, &msg->ip );
        cfg_v_set( CFG_PARAM_IP_SUBNET_MASK, &msg->subnet );
        cfg_v_set( CFG_PARAM_DNS_SERVER, &msg->dns );

        wifi_rx_udp_fifo_overruns   = msg->rx_udp_fifo_overruns;
        wifi_rx_udp_port_overruns   = msg->rx_udp_port_overruns;
        wifi_udp_received           = msg->udp_received;
        wifi_udp_sent               = msg->udp_sent;
        wifi_comm_errors            = msg->comm_errors;
        mem_heap_peak               = msg->mem_heap_peak;

        intf_max_time               = msg->intf_max_time;
        vm_max_time                 = msg->vm_max_time;
        wifi_max_time               = msg->wifi_max_time;
        mem_max_time                = msg->mem_max_time;
    }
    else if( header->data_id == WIFI_DATA_ID_DEBUG ){

        if( header->len != sizeof(wifi_msg_debug_t) ){

            goto len_error;
        }

        wifi_msg_debug_t *msg = (wifi_msg_debug_t *)data;

        log_v_debug_P( PSTR("ESP free heap: %u"), msg->free_heap );
    }
    else if( header->data_id == WIFI_DATA_ID_UDP_HEADER ){

        if( header->len != sizeof(wifi_msg_udp_header_t) ){

            goto len_error;
        }

        wifi_msg_udp_header_t *msg = (wifi_msg_udp_header_t *)data;
        
        // check if port is busy
        if( sock_b_port_busy( msg->rport ) ){

            log_v_debug_P( PSTR("port busy: %u"), msg->rport );
            goto error;
        }

        // check if we have a netmsg that didn't get freed for some reason
        if( rx_netmsg > 0 ){

            log_v_debug_P( PSTR("freeing loose netmsg") );     

            netmsg_v_release( rx_netmsg );
            rx_netmsg = -1;
        }

        // allocate netmsg
        rx_netmsg = netmsg_nm_create( NETMSG_TYPE_UDP );

        if( rx_netmsg < 0 ){

            log_v_debug_P( PSTR("rx udp no netmsg") );     

            goto error;
        }

        netmsg_state_t *state = netmsg_vp_get_state( rx_netmsg );

        // allocate data buffer
        state->data_handle = mem2_h_alloc2( msg->len, MEM_TYPE_SOCKET_BUFFER );

        if( state->data_handle < 0 ){

            log_v_debug_P( PSTR("rx udp no handle") );     

            netmsg_v_release( rx_netmsg );
            rx_netmsg = 0;

            goto error;
        }        

        // set up address info
        state->laddr.port   = msg->lport;
        state->raddr.port   = msg->rport;
        state->raddr.ipaddr = msg->addr;

        rx_netmsg_crc       = msg->crc;

        rx_netmsg_index = 0;
    }
    else if( header->data_id == WIFI_DATA_ID_UDP_DATA ){

        if( rx_netmsg <= 0 ){

            log_v_debug_P( PSTR("rx udp no netmsg") );     

            goto error;
        }

        netmsg_state_t *state = netmsg_vp_get_state( rx_netmsg );
        uint8_t *ptr = mem2_vp_get_ptr( state->data_handle );        
        uint16_t total_len = mem2_u16_get_size( state->data_handle );

        // bounds check
        if( ( header->len + rx_netmsg_index ) > total_len ){

            log_v_debug_P( PSTR("rx udp len error") );     

            // bad length, throwaway
            netmsg_v_release( rx_netmsg );
            rx_netmsg = 0;

            goto error;
        }

        memcpy( &ptr[rx_netmsg_index], data, header->len );

        rx_netmsg_index += header->len;

        // message is complete
        if( rx_netmsg_index == total_len ){

            // check crc
            if( crc_u16_block( ptr, total_len ) != rx_netmsg_crc ){

                netmsg_v_release( rx_netmsg );
                rx_netmsg = 0;

                log_v_debug_P( PSTR("rx udp crc error") );     

                goto error;
            }

            netmsg_v_receive( rx_netmsg );
            rx_netmsg = 0;
        }   
    }
    // else if( header->data_id == WIFI_DATA_ID_WIFI_SCAN_RESULTS ){
    
    //     if( wifi_networks_handle < 0 ){        

    //         wifi_networks_handle = mem2_h_alloc( sizeof(wifi_msg_scan_results_t) );

    //         if( wifi_networks_handle > 0 ){

    //             memcpy( mem2_vp_get_ptr( wifi_networks_handle ), data, sizeof(wifi_msg_scan_results_t) );
    //         }
    //     }

    //     // wifi_msg_scan_results_t *msg = (wifi_msg_scan_results_t *)data;

    //     // for( uint8_t i = 0; i < msg->count; i++ ){

    //     //     log_v_debug_P(PSTR("%ld %lu"), msg->networks[i].rssi, msg->networks[i].ssid_hash );
    //     // }
    // }
    // check if msg handler is installed
    else if( wifi_i8_msg_handler ){

        wifi_i8_msg_handler( header->data_id, data, header->len );
    }

    return 0;

len_error:

    log_v_debug_P( PSTR("Wifi len error") );
    return -3;    

error:
    return -4;    
}


PT_THREAD( wifi_comm_thread( pt_t *pt, void *state ) )
{
PT_BEGIN( pt );
    
restart:
    
    _wifi_v_enter_normal_mode();

    ATOMIC;
    wifi_rx_ready = FALSE;
    END_ATOMIC;

    // delay while wifi boots up
    TMR_WAIT( pt, 300 );

    enable_irq();
    
    wifi_v_reset_comm();

    TMR_WAIT( pt, 100 );

    if( !wifi_b_comm_ready() ){

        goto restart;
    }

    wifi_status = WIFI_STATE_ALIVE;

    // log_v_debug_P( PSTR("Wifi RX ready") );


    // set ready and wait for message
    wifi_v_set_rx_ready();

        
    while(1){

        THREAD_WAIT_WHILE( pt, wifi_u8_get_control_byte() == WIFI_COMM_IDLE );

        uint8_t control_byte = wifi_u8_get_control_byte();

        if( control_byte == WIFI_COMM_DATA ){

            thread_v_set_alarm( tmr_u32_get_system_time_ms() + 20 );    
            THREAD_WAIT_WHILE( pt, ( process_rx_data() < 0 ) &&
                                   ( thread_b_alarm_set() ) );

            if( thread_b_alarm() ){

                log_v_debug_P( PSTR("Wifi rx timeout") );

                wifi_v_set_rx_ready();
            }
        }
        else if( control_byte == WIFI_COMM_QUERY_READY ){

            log_v_debug_P( PSTR("query ready") );
            wifi_v_set_rx_ready();
        }
        else{
            log_v_debug_P( PSTR("control: %x %c"), control_byte, control_byte );
            wifi_v_set_rx_ready();
        }
    
        THREAD_YIELD( pt );
    }

PT_END( pt );
}


PT_THREAD( wifi_status_thread( pt_t *pt, void *state ) )
{
PT_BEGIN( pt );

    while(1){

        THREAD_WAIT_WHILE( pt, !wifi_b_connected() );

        TMR_WAIT( pt, 1000 );

        wifi_uptime += 1;

        if( !wifi_b_connected() ){

            wifi_uptime = 0;
        }
    }

PT_END( pt );
}



PT_THREAD( wifi_loader_thread( pt_t *pt, loader_thread_state_t *state ) )
{
PT_BEGIN( pt );

    state->fw_file = 0;

    // log_v_debug_P( PSTR("wifi loader starting") );

    
restart:

    _wifi_v_enter_boot_mode();
    wifi_status = WIFI_STATE_BOOT;

    if( state->fw_file == 0 ){

        state->fw_file = fs_f_open_P( PSTR("wifi_firmware.bin"), FS_MODE_READ_ONLY );
    }

    if( state->fw_file <= 0 ){

        log_v_debug_P( PSTR("Wifi firmware image not found! Trying to start anyway...") );

        goto run_wifi;
    }

    // delay while wifi boots up
    TMR_WAIT( pt, 250 );

    state->timeout = 10;
    while( state->timeout > 0 ){

        _wifi_v_usart_flush();

        state->timeout--;

        if( state->timeout == 0 ){

            log_v_debug_P( PSTR("wifi loader timeout") );
            goto restart;
        }

        uint8_t buf[32];
        memset( buf, 0xff, sizeof(buf) );

        // ESP seems to miss the first sync for some reason,
        // so we'll just send twice.
        // it's not really a big deal from a timing standpoint since
        // we'd try again in a few milliseconds, but if the wait response
        // function is doing error logging, it saves us a pointless error
        // message on every start up.
        esp_v_send_sync();
        esp_v_send_sync();

        // blocking wait!
        int8_t status = esp_i8_wait_response( buf, sizeof(buf), ESP_SYNC_TIMEOUT );

        if( status == 0 ){

            esp_response_t *resp = (esp_response_t *)buf;

            if( resp->opcode == ESP_SYNC ){

                break;
            }
        }

        TMR_WAIT( pt, 5 );
    }

    // delay, as Sync will output several responses
    TMR_WAIT( pt, 50 );

    int8_t status = esp_i8_load_cesanta_stub();

    if( status < 0 ){

        log_v_debug_P( PSTR("error %d"), status );

        TMR_WAIT( pt, 500 );
        goto restart;
    }

    // change baud rate
    usart_v_set_baud( &WIFI_USART, ESP_CESANTA_BAUD_USART_SETTING );
    _wifi_v_usart_flush();

    memset( rx_buf, 0, sizeof(rx_buf) );
    enable_rx_dma();

    // cesanta stub has a delay, so make sure we wait plenty long enough
    _delay_ms( 50 );

    // now check buffer, Cesanta will send us a hello message
    if( !( ( rx_buf[0] == SLIP_END ) &&
           ( rx_buf[1] == 'O' ) &&
           ( rx_buf[2] == 'H' ) &&
           ( rx_buf[3] == 'A' ) &&
           ( rx_buf[4] == 'I' ) &&
           ( rx_buf[5] == SLIP_END ) ) ){

        log_v_debug_P( PSTR("error") );

        disable_rx_dma();

        TMR_WAIT( pt, 500 );
        goto error;
    }

    status_led_v_set( 1, STATUS_LED_BLUE );

    // log_v_debug_P( PSTR("Cesanta flasher ready!") );

    uint32_t file_len;
    cfg_i8_get( CFG_PARAM_WIFI_FW_LEN, &file_len );

    uint8_t wifi_digest[MD5_LEN];

    int8_t md5_status = esp_i8_md5( file_len, wifi_digest );
    if( md5_status < 0 ){

        log_v_debug_P( PSTR("error %d"), md5_status );

        TMR_WAIT( pt, 1000 );
        goto restart;
    }

    fs_v_seek( state->fw_file, file_len );

    uint8_t file_digest[MD5_LEN];
    memset( file_digest, 0, MD5_LEN );
    fs_i16_read( state->fw_file, file_digest, MD5_LEN );

    uint8_t cfg_digest[MD5_LEN];
    cfg_i8_get( CFG_PARAM_WIFI_MD5, cfg_digest );


    if( memcmp( file_digest, cfg_digest, MD5_LEN ) == 0 ){

        // file and cfg match, so our file is valid


        if( memcmp( file_digest, wifi_digest, MD5_LEN ) == 0 ){

            // all 3 match, run wifi
            // log_v_debug_P( PSTR("Wifi firmware image valid") );

            goto run_wifi;
        }
        else{
            // wifi does not match file - need to load

            goto load_image;
        }
    }

    if( memcmp( wifi_digest, cfg_digest, MD5_LEN ) == 0 ){

        // wifi matches cfg, this is ok, run.
        // maybe the file is bad.
        log_v_debug_P( PSTR("Wifi MD5 mismatch, possible bad file load") );

        goto run_wifi;
    }

    if( memcmp( wifi_digest, file_digest, MD5_LEN ) == 0 ){

        // in this case, file matches wifi, so our wifi image is valid
        // and so is our file.
        // but our cfg is mismatched.
        // so we'll restore it and then run the wifi

        cfg_v_set( CFG_PARAM_WIFI_MD5, file_digest );

        log_v_debug_P( PSTR("Wifi MD5 mismatch, restored from file") );

        goto run_wifi;
    }

    // probably don't want to actually assert here...

    // try to run anyway
    goto run_wifi;


    WIFI_PD_PORT.OUTCLR = ( 1 << WIFI_PD_PIN ); // hold chip in reset

    log_v_debug_P( PSTR("error") );

    TMR_WAIT( pt, 50000000 );

    ASSERT( FALSE );

load_image:

    usb_v_detach();

    log_v_debug_P( PSTR("Loading wifi image...") );


    if( esp_i8_load_flash( state->fw_file ) < 0 ){

        log_v_debug_P( PSTR("error") );
        goto error;
    }

    memset( wifi_digest, 0xff, MD5_LEN );

    if( esp_i8_md5( file_len, wifi_digest ) < 0 ){

        log_v_debug_P( PSTR("error") );
        goto restart;
    }

    // verify

    log_v_debug_P( PSTR("Wifi flash load done") );

    if( state->fw_file > 0 ){

        fs_f_close( state->fw_file );
    }

    // restart
    sys_v_reboot_delay( SYS_MODE_NORMAL );

    THREAD_EXIT( pt );

error:

    WIFI_PD_PORT.OUTCLR = ( 1 << WIFI_PD_PIN ); // hold chip in reset

    status_led_v_set( 1, STATUS_LED_RED );

    if( state->fw_file > 0 ){

        fs_f_close( state->fw_file );
    }


    log_v_debug_P( PSTR("wifi load fail") );

    THREAD_EXIT( pt );

run_wifi:

    thread_t_create( wifi_comm_thread,
                     PSTR("wifi_comm"),
                     0,
                     0 );

    thread_t_create( wifi_connection_manager_thread,
                     PSTR("wifi_connection_manager"),
                     0,
                     0 );

    if( state->fw_file > 0 ){

        fs_f_close( state->fw_file );
    }

PT_END( pt );
}

PT_THREAD( wifi_echo_thread( pt_t *pt, void *state ) )
{
PT_BEGIN( pt );

    static socket_t sock;

    sock = sock_s_create( SOCK_DGRAM );
    sock_v_bind( sock, 7 );

    while(1){

        // sock_addr_t raddr;
        // raddr.ipaddr = ip_a_addr(255,255,255,255);
        // raddr.port = 12345;
        // uint8_t data[256];
        //
        // sock_i16_sendto( sock, data, sizeof(data), &raddr );
        //
        // THREAD_YIELD( pt );

        THREAD_WAIT_WHILE( pt, sock_i8_recvfrom( sock ) < 0 );

        wifi_echo_recv++;

        EVENT( EVENT_ID_DEBUG_0, 2 );

        if( sock_i16_sendto( sock, sock_vp_get_data( sock ), sock_i16_get_bytes_read( sock ), 0 ) >= 0 ){

            wifi_echo_sent++;
        }
    }

PT_END( pt );
}


int8_t get_route( ip_addr_t *subnet, ip_addr_t *subnet_mask ){

    // check if interface is up
    if( !wifi_b_connected() ){

        return -1;
    }

    cfg_i8_get( CFG_PARAM_IP_ADDRESS, subnet );
    cfg_i8_get( CFG_PARAM_IP_SUBNET_MASK, subnet_mask );

    return 0;
}

ROUTING_TABLE routing_table_entry_t route_wifi = {
    get_route,
    wifi_i8_send_udp,
    open_close_port,
    ROUTE_FLAGS_ALLOW_GLOBAL_BROADCAST
};


void wifi_v_init( void ){

    wifi_status = WIFI_STATE_BOOT;

    // enable DMA controller
    DMA.CTRL |= DMA_ENABLE_bm;


    // set up IO
    WIFI_PD_PORT.DIRSET = ( 1 << WIFI_PD_PIN );
    WIFI_SS_PORT.DIRCLR = ( 1 << WIFI_SS_PIN );
    WIFI_USART_XCK_PORT.DIRCLR = ( 1 << WIFI_USART_XCK_PIN );
    WIFI_BOOT_PORT.DIRCLR = ( 1 << WIFI_BOOT_PIN );

    WIFI_PD_PORT.OUTCLR = ( 1 << WIFI_PD_PIN ); // hold chip in reset
    WIFI_BOOT_PORT.WIFI_BOOT_PINCTRL            = PORT_OPC_PULLDOWN_gc;
    WIFI_SS_PORT.WIFI_SS_PINCTRL                = PORT_OPC_PULLDOWN_gc;
    WIFI_USART_XCK_PORT.WIFI_USART_XCK_PINCTRL  = PORT_OPC_PULLDOWN_gc;
    WIFI_USART_XCK_PORT.OUTCLR                  = ( 1 << WIFI_USART_XCK_PIN );

    // return;
     // if( sys_u8_get_mode() == SYS_MODE_SAFE ){

     //     return;
     // }

     thread_t_create( THREAD_CAST(wifi_loader_thread),
                      PSTR("wifi_loader"),
                      0,
                      sizeof(loader_thread_state_t) );


    thread_t_create( wifi_status_thread,
                          PSTR("wifi_status"),
                          0,
                          0 );

    thread_t_create( wifi_echo_thread,
                        PSTR("wifi_echo"),
                        0,
                        0 );
}

bool wifi_b_connected( void ){

    return ( wifi_status_reg & WIFI_STATUS_CONNECTED ) != 0;
}

bool wifi_b_ap_mode( void ){

    return ( wifi_status_reg & WIFI_STATUS_AP_MODE ) != 0;
}

bool wifi_b_ap_mode_enabled( void ){

    if( default_ap_mode ){

        return TRUE;
    }

    bool wifi_enable_ap = FALSE;
    cfg_i8_get( CFG_PARAM_WIFI_ENABLE_AP, &wifi_enable_ap );
    
    return wifi_enable_ap;    
}

bool wifi_b_attached( void ){

    return wifi_status >= WIFI_STATE_ALIVE;
}

int8_t wifi_i8_get_status( void ){

    return wifi_status;
}

uint32_t wifi_u32_get_received( void ){

    return wifi_udp_received;
}



ISR(WIFI_IRQ_VECTOR){
// OS_IRQ_BEGIN(WIFI_IRQ_VECTOR);

    wifi_rx_ready = TRUE;

// OS_IRQ_END();
}


bool wifi_b_running( void ){

    return TRUE;
}
