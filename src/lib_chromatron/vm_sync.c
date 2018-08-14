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

#include "sapphire.h"

#include "hash.h"
#include "esp8266.h"
#include "vm_wifi_cmd.h"

KV_SECTION_META kv_meta_t vm_sync_kv[] = {
    { SAPPHIRE_TYPE_STRING32, 0, KV_FLAGS_PERSIST,   0, 0, "gfx_sync_group" },
};

static int16_t data_remaining = -1;

PT_THREAD( vm_sync_thread( pt_t *pt, void *state ) );

uint32_t vm_sync_u32_get_sync_group_hash( void ){

    char sync_group[32];
    if( kv_i8_get( __KV__gfx_sync_group, sync_group, sizeof(sync_group) ) < 0 ){

        return 0;
    }

    return hash_u32_string( sync_group );    
}


int8_t vm_sync_i8_request_frame_sync( void ){

	return wifi_i8_send_msg( WIFI_DATA_ID_REQUEST_FRAME_SYNC, 0, 0 );
}


static void write_to_sync_file( uint16_t offset, uint8_t *data, uint16_t len ){

	file_t f = fs_f_open_P( PSTR("vm_sync"), FS_MODE_WRITE_APPEND );

	if( f < 0 ){

		return;
	}

	// check offset
	if( ( fs_i32_get_size( f ) - sizeof(wifi_msg_vm_frame_sync_t) ) != offset ){

		goto done;
	}

	fs_i16_write( f, data, len );

	data_remaining -= len;

done:
	f = fs_f_close( f ); 
}

void vm_sync_v_process_msg( uint8_t data_id, uint8_t *data, uint16_t len ){

	if( data_id == WIFI_DATA_ID_VM_FRAME_SYNC ){

		if( len != sizeof(wifi_msg_vm_frame_sync_t) ){

			return;
		}

		wifi_msg_vm_frame_sync_t *msg = (wifi_msg_vm_frame_sync_t *)data;

		data_remaining = msg->data_len;

		// delete file and recreate
		file_id_t8 id = fs_i8_get_file_id_P( PSTR("vm_sync") );

		if( id > 0 ){

			fs_i8_delete_id( id );
		}

		file_t f = fs_f_open_P( PSTR("vm_sync"), FS_MODE_WRITE_OVERWRITE | FS_MODE_CREATE_IF_NOT_FOUND );

		if( f < 0 ){

			return;
		}

		fs_i16_write( f, msg, sizeof(wifi_msg_vm_frame_sync_t) );

		f = fs_f_close( f ); 
    }
    else if( data_id == WIFI_DATA_ID_VM_SYNC_DATA ){

        wifi_msg_vm_sync_data_t *msg = (wifi_msg_vm_sync_data_t *)data;
        data += sizeof(wifi_msg_vm_sync_data_t);

        log_v_debug_P( PSTR("sync offset: %u"), msg->offset );

        write_to_sync_file( msg->offset, data, len - sizeof(wifi_msg_vm_sync_data_t) );
    }
}



PT_THREAD( vm_sync_thread( pt_t *pt, void *state ) )
{
PT_BEGIN( pt );
    	
    while( TRUE ){

    	THREAD_WAIT_WHILE( pt, vm_sync_u32_get_sync_group_hash() == 0 );

    	THREAD_WAIT_WHILE( pt, data_remaining != 0 );


    }

PT_END( pt );
}



void vm_sync_v_init( void ){

    thread_t_create( vm_sync_thread,
                    PSTR("vm_sync"),
                    0,
                    0 );    
}


