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

#include "system.h"
#include "threading.h"
#include "logging.h"
#include "timers.h"
#include "keyvalue.h"
#include "fs.h"
#include "random.h"
#include "graphics.h"
#include "esp8266.h"
#include "hash.h"
#include "kvdb.h"
#include "vm_wifi_cmd.h"

#include "vm.h"
#include "vm_core.h"
#include "vm_cron.h"


static bool vm_reset[VM_MAX_VMS];
static bool vm_run[VM_MAX_VMS];

static int8_t vm_status[VM_MAX_VMS];
static uint16_t vm_loop_time[VM_MAX_VMS];
static uint16_t vm_thread_time[VM_MAX_VMS];
static uint16_t vm_max_cycles[VM_MAX_VMS];
static uint16_t vm_sizes[VM_MAX_VMS];

static uint8_t vm_active_threads[VM_MAX_VMS];


int8_t vm_i8_kv_handler(
    kv_op_t8 op,
    catbus_hash_t32 hash,
    void *data,
    uint16_t len ){

    if( op == KV_OP_SET ){

    }
    else if( op == KV_OP_GET ){

        if( hash == __KV__vm_isa ){

            *(uint8_t *)data = VM_ISA_VERSION;
        }
        else if( hash == __KV__vm_total_size ){

            uint16_t *total_size = (uint16_t *)data;
            *total_size = 0;

            for( uint8_t i = 0; i < VM_MAX_VMS; i++ ){

                *total_size += vm_sizes[i];
            }
        }
    }

    return 0;
}


KV_SECTION_META kv_meta_t vm_info_kv[] = {
    { SAPPHIRE_TYPE_BOOL,     0, 0,                   &vm_reset[0],          0,                  "vm_reset" },
    { SAPPHIRE_TYPE_BOOL,     0, KV_FLAGS_PERSIST,    &vm_run[0],            0,                  "vm_run" },
    { SAPPHIRE_TYPE_STRING32, 0, KV_FLAGS_PERSIST,    0,                     0,                  "vm_prog" },
    { SAPPHIRE_TYPE_INT8,     0, KV_FLAGS_READ_ONLY,  &vm_status[0],         0,                  "vm_status" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_loop_time[0],      0,                  "vm_loop_time" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_thread_time[0],    0,                  "vm_thread_time" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_max_cycles[0],     0,                  "vm_peak_cycles" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_sizes[0],          0,                  "vm_size" },

    { SAPPHIRE_TYPE_BOOL,     0, 0,                   &vm_reset[1],          0,                  "vm_reset_1" },
    { SAPPHIRE_TYPE_BOOL,     0, KV_FLAGS_PERSIST,    &vm_run[1],            0,                  "vm_run_1" },
    { SAPPHIRE_TYPE_STRING32, 0, KV_FLAGS_PERSIST,    0,                     0,                  "vm_prog_1" },
    { SAPPHIRE_TYPE_INT8,     0, KV_FLAGS_READ_ONLY,  &vm_status[1],         0,                  "vm_status_1" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_loop_time[1],      0,                  "vm_loop_time_1" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_thread_time[1],    0,                  "vm_thread_time_1" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_max_cycles[1],     0,                  "vm_peak_cycles_1" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_sizes[1],          0,                  "vm_size_1" },

    { SAPPHIRE_TYPE_BOOL,     0, 0,                   &vm_reset[2],          0,                  "vm_reset_2" },
    { SAPPHIRE_TYPE_BOOL,     0, KV_FLAGS_PERSIST,    &vm_run[2],            0,                  "vm_run_2" },
    { SAPPHIRE_TYPE_STRING32, 0, KV_FLAGS_PERSIST,    0,                     0,                  "vm_prog_2" },
    { SAPPHIRE_TYPE_INT8,     0, KV_FLAGS_READ_ONLY,  &vm_status[2],         0,                  "vm_status_2" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_loop_time[2],      0,                  "vm_loop_time_2" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_thread_time[2],    0,                  "vm_thread_time_2" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_max_cycles[2],     0,                  "vm_peak_cycles_2" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_sizes[2],          0,                  "vm_size_2" },

    { SAPPHIRE_TYPE_BOOL,     0, 0,                   &vm_reset[3],          0,                  "vm_reset_3" },
    { SAPPHIRE_TYPE_BOOL,     0, KV_FLAGS_PERSIST,    &vm_run[3],            0,                  "vm_run_3" },
    { SAPPHIRE_TYPE_STRING32, 0, KV_FLAGS_PERSIST,    0,                     0,                  "vm_prog_3" },
    { SAPPHIRE_TYPE_INT8,     0, KV_FLAGS_READ_ONLY,  &vm_status[3],         0,                  "vm_status_3" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_loop_time[3],      0,                  "vm_loop_time_3" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_thread_time[3],    0,                  "vm_thread_time_3" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_max_cycles[3],     0,                  "vm_peak_cycles_3" },
    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  &vm_sizes[3],          0,                  "vm_size_3" },

    { SAPPHIRE_TYPE_UINT16,   0, KV_FLAGS_READ_ONLY,  0,                     vm_i8_kv_handler,   "vm_total_size" },
    { SAPPHIRE_TYPE_UINT8,    0, KV_FLAGS_READ_ONLY,  0,                     vm_i8_kv_handler,   "vm_isa" },
};

typedef struct{
    uint8_t vm_id;
    uint8_t thread_id;
    uint16_t func_addr;
} vm_thread_state_t;

PT_THREAD( vm_thread( pt_t *pt, vm_thread_state_t *state ) );

// keys that we really don't want the VM be to be able to write to.
// generally, these are going to be things that would allow it to 
// brick hardware, mess up the wifi connection, or mess up the pixel 
// array.
static const PROGMEM uint32_t restricted_keys[] = {
    __KV__reboot,
    __KV__wifi_enable_ap,
    __KV__wifi_fw_len,
    __KV__wifi_md5,
    __KV__wifi_router,
    __KV__pix_clock,
    __KV__pix_count,
    __KV__pix_mode,
    __KV__catbus_data_port,    
};


static catbus_hash_t32 get_link_tag( uint8_t vm_id ){

    catbus_hash_t32 link_tag = 0;

    if( vm_id == 0 ){

        link_tag = __KV__vm_0;
    }
    else if( vm_id == 1 ){

        link_tag = __KV__vm_1;
    }
    else if( vm_id == 2 ){

        link_tag = __KV__vm_2;
    }
    else if( vm_id == 3 ){

        link_tag = __KV__vm_3;
    }
    else{

        ASSERT( FALSE );
    }

    return link_tag;
}


static void reset_published_data( uint8_t vm_id ){

    gfx_v_reset_subscribed( 1 << vm_id );
    kvdb_v_clear_tag( 0, 1 << vm_id );
    catbus_v_purge_links( get_link_tag( vm_id ) );
} 

static file_t get_program_handle( catbus_hash_t32 hash ){

    char program_fname[KV_NAME_LEN];

    if( kv_i8_get( hash, program_fname, sizeof(program_fname) ) < 0 ){

        return -1;
    }

    file_t f = fs_f_open( program_fname, FS_MODE_READ_ONLY );

    if( f < 0 ){

        return -1;
    }

    return f;
}


#ifdef VM_TARGET_ESP

static int8_t send_reset_message( uint8_t vm_id ){

    wifi_msg_reset_vm_t reset_msg;
    reset_msg.vm_id = vm_id;

    return wifi_i8_send_msg( WIFI_DATA_ID_RESET_VM, (uint8_t *)&reset_msg, sizeof(reset_msg) );
}


static int8_t _vm_i8_run_vm( uint8_t vm_id, uint8_t data_id, uint16_t func_addr, wifi_msg_vm_info_t *info ){

    // synchronize database parameters
    gfx_v_sync_db( FALSE );

    wifi_msg_vm_run_t msg;
    msg.vm_id = vm_id;
    msg.func_addr = func_addr;
    if( wifi_i8_send_msg( data_id, (uint8_t *)&msg, sizeof(msg) ) < 0 ){

        return VM_STATUS_COMM_ERROR;
    }

    if( wifi_i8_receive_msg( WIFI_DATA_ID_VM_INFO, (uint8_t *)info, sizeof(wifi_msg_vm_info_t), 0 ) < 0 ){

        return VM_STATUS_COMM_ERROR;
    }

    for( uint8_t i = 0; i < VM_MAX_THREADS; i++ ){

        // check if starting a new thread
        if( ( info->thread_delays[i] >= 0 ) && ( ( vm_active_threads[vm_id] & ( 1 << i ) ) == 0 ) ){

            vm_active_threads[vm_id] |= ( 1 << i );

            vm_thread_state_t state;
            state.vm_id         = vm_id;
            state.thread_id     = i;

            thread_t_create( THREAD_CAST(vm_thread),
                             PSTR("vm_thread"),
                             &state,
                             sizeof(state) );
        }
        // check if thread stopped
        else if( ( info->thread_delays[i] < 0 ) && ( ( vm_active_threads[vm_id] & ( 1 << i ) ) != 0 ) ){

            log_v_debug_P( PSTR("stopped thread %d"), i );    

            vm_active_threads[vm_id] &= ~( 1 << i );
        }
    }

    // read database
    gfx_v_read_db();

    return info->status;
}

// static void reset_vm( uint8_t vm_id ){

//     send_reset_message( vm_id );
//     reset_published_data( vm_id );
//     vm_cron_v_unload( vm_id );
//     vm_status[vm_id] = VM_STATUS_NOT_RUNNING;

//     vm_loop_time[vm_id]     = 0;
//     vm_thread_time[vm_id]   = 0;
//     vm_max_cycles[vm_id]    = 0;      
// }

static int8_t load_vm_wifi( uint8_t vm_id ){

    uint32_t start_time = tmr_u32_get_system_time_ms();

    wifi_msg_load_vm_t vm_load_msg;

    catbus_hash_t32 hash = 0;
    catbus_hash_t32 link_tag = get_link_tag( vm_id );
    
    if( vm_id == 0 ){

        hash = __KV__vm_prog;
    }
    else if( vm_id == 1 ){

        hash = __KV__vm_prog_1;
    }
    else if( vm_id == 2 ){

        hash = __KV__vm_prog_2;
    }
    else if( vm_id == 3 ){

        hash = __KV__vm_prog_3;
    }
    else{

        ASSERT( FALSE );
    }

    vm_loop_time[vm_id]     = 0;
    vm_thread_time[vm_id]   = 0;
    vm_max_cycles[vm_id]    = 0;

    file_t f = get_program_handle( hash );

    if( f < 0 ){

        return -1;
    }

    if( send_reset_message( vm_id ) < 0 ){

        goto error;
    }

    reset_published_data( vm_id );

    // sync graphics parameters, because script init function will run as soon as loading is complete.
    gfx_v_sync_params();
    
    log_v_debug_P( PSTR("Loading VM: %d"), vm_id );

    // file found, get program size from file header
    int32_t vm_size;
    fs_i16_read( f, (uint8_t *)&vm_size, sizeof(vm_size) );

    // sanity check
    if( vm_size > VM_MAX_IMAGE_SIZE ){

        goto error;
    }

    int32_t vm_size_copy = vm_size;

    fs_v_seek( f, 0 );    
    int32_t check_len = fs_i32_get_size( f ) - sizeof(uint32_t);

    uint32_t computed_file_hash = hash_u32_start();

    // check file hash
    while( check_len > 0 ){

        uint16_t copy_len = sizeof(vm_load_msg.chunk);

        if( copy_len > check_len ){

            copy_len = check_len;
        }

        int16_t read = fs_i16_read( f, vm_load_msg.chunk, copy_len );

        if( read < 0 ){

            // this should not happen. famous last words.
            goto error;
        }

        // update hash
        computed_file_hash = hash_u32_partial( computed_file_hash, vm_load_msg.chunk, copy_len );
        
        check_len -= read;
    }

    // read file hash
    uint32_t file_hash = 0;
    fs_i16_read( f, (uint8_t *)&file_hash, sizeof(file_hash) );

    // check hashes
    if( file_hash != computed_file_hash ){

        log_v_debug_P( PSTR("VM load error: %d"), VM_STATUS_ERR_BAD_FILE_HASH );
        goto error;
    }

    // read header
    fs_v_seek( f, sizeof(vm_size) );    
    vm_program_header_t header;
    fs_i16_read( f, (uint8_t *)&header, sizeof(header) );

    vm_state_t state;

    int8_t status = vm_i8_load_program( VM_LOAD_FLAGS_CHECK_HEADER, (uint8_t *)&header, sizeof(header), &state );

    if( status < 0 ){

        log_v_debug_P( PSTR("VM load error: %d"), status );
        goto error;
    }

    // seek back to program start
    fs_v_seek( f, sizeof(vm_size) );

    vm_load_msg.total_size  = vm_size;
    vm_load_msg.offset      = 0;

    while( vm_size > 0 ){

        uint16_t copy_len = sizeof(vm_load_msg.chunk);

        if( copy_len > vm_size ){

            copy_len = vm_size;
        }

        int16_t read = fs_i16_read( f, vm_load_msg.chunk, copy_len );

        if( read < 0 ){

            // this should not happen. famous last words.
            goto error;
        }

        vm_load_msg.vm_id = vm_id;
        vm_load_msg.total_size = vm_size;
        uint16_t msg_len = read + sizeof(vm_load_msg.vm_id) + sizeof(vm_load_msg.total_size) + sizeof(vm_load_msg.offset);
        if( wifi_i8_send_msg( WIFI_DATA_ID_LOAD_VM, (uint8_t *)&vm_load_msg, msg_len ) < 0 ){

            // comm error
            goto error;
        }
        
        vm_load_msg.offset += read;
        vm_size -= read;
    }

    // read magic number
    uint32_t meta_magic = 0;
    fs_i16_read( f, (uint8_t *)&meta_magic, sizeof(meta_magic) );

    if( meta_magic == META_MAGIC ){

        char meta_string[KV_NAME_LEN];
        memset( meta_string, 0, sizeof(meta_string) );

        // skip first string, it's the script name
        fs_v_seek( f, fs_i32_tell( f ) + sizeof(meta_string) );

        // load meta names to database lookup
        while( fs_i16_read( f, meta_string, sizeof(meta_string) ) == sizeof(meta_string) ){
        
            kvdb_v_set_name( meta_string );
            
            memset( meta_string, 0, sizeof(meta_string) );
        }
    }
    else{

        log_v_debug_P( PSTR("Meta read failed") );

        goto error;
    }


    catbus_meta_t meta;
    
    // set up additional DB entries
    fs_v_seek( f, sizeof(vm_size) + state.db_start );

    for( uint8_t i = 0; i < state.db_count; i++ ){

        fs_i16_read( f, (uint8_t *)&meta, sizeof(meta) );

        kvdb_i8_add( meta.hash, meta.type, meta.count + 1, 0, 0 );
        kvdb_v_set_tag( meta.hash, 1 << vm_id );      

        gfx_v_subscribe_key( meta.hash, ( 1 << vm_id ) );
    }   


    // read through database keys
    uint32_t read_key_hash = 0;

    fs_v_seek( f, sizeof(vm_size) + state.read_keys_start );

    for( uint16_t i = 0; i < state.read_keys_count; i++ ){

        fs_i16_read( f, (uint8_t *)&read_key_hash, sizeof(uint32_t) );

        gfx_v_subscribe_key( read_key_hash, ( 1 << vm_id ) );
    }
    

    // check published keys and add to DB
    fs_v_seek( f, sizeof(vm_size) + state.publish_start );

    for( uint8_t i = 0; i < state.publish_count; i++ ){

        vm_publish_t publish;

        fs_i16_read( f, (uint8_t *)&publish, sizeof(publish) );

        // TODO
        // this dirty hack brought to you by:
        // not supporting variable length strings!

        // for now, the compiler defaults to string512 for the type
        // of all strings.  since we don't normally need strings that large
        // and more importantly, have no where near enough ram to actually 
        // use them, we'll constrain them to 64 bytes until (if?) we have
        // an actual fix for this.
        if( publish.type == CATBUS_TYPE_STRING512 ){

            publish.type = CATBUS_TYPE_STRING64;
        }

        kvdb_i8_add( publish.hash, publish.type, 1, 0, 0 );
        kvdb_v_set_tag( publish.hash, ( 1 << vm_id ) );

        gfx_v_subscribe_key( publish.hash, ( 1 << vm_id ) );
    }   

    // check write keys
    fs_v_seek( f, sizeof(vm_size) + state.write_keys_start );

    for( uint8_t i = 0; i < state.write_keys_count; i++ ){

        uint32_t write_hash = 0;
        fs_i16_read( f, (uint8_t *)&write_hash, sizeof(write_hash) );

        if( write_hash == 0 ){

            continue;
        }

        // check if writing to restricted key
        for( uint8_t j = 0; j < cnt_of_array(restricted_keys); j++ ){

            uint32_t restricted_key = 0;
            memcpy_P( (uint8_t *)&restricted_key, &restricted_keys[j], sizeof(restricted_key) );

            if( restricted_key == 0 ){

                continue;
            }   

            // check for match
            if( restricted_key == write_hash ){

                vm_status[vm_id] = VM_STATUS_RESTRICTED_KEY;

                log_v_debug_P( PSTR("Restricted key: %lu"), write_hash );

                goto error;
            }
        }

        gfx_v_subscribe_key( write_hash, ( 1 << vm_id ) );   
    }

    // set up links
    fs_v_seek( f, sizeof(vm_size) + state.link_start );

    for( uint8_t i = 0; i < state.link_count; i++ ){

        link_t link;

        fs_i16_read( f, (uint8_t *)&link, sizeof(link) );

        if( link.send ){

            catbus_l_send( link.source_hash, link.dest_hash, &link.query, link_tag, 0 );
        }
        else{

            catbus_l_recv( link.dest_hash, link.source_hash, &link.query, link_tag, 0 );
        }
    }

    // load cron jobs
    vm_cron_v_load( vm_id, &state, f );

    // send len 0 to indicate load complete.
    uint16_t msg_len = sizeof(vm_load_msg.vm_id) + sizeof(vm_load_msg.total_size) + sizeof(vm_load_msg.offset);
    vm_load_msg.vm_id = vm_id;
    if( wifi_i8_send_msg( WIFI_DATA_ID_LOAD_VM, (uint8_t *)&vm_load_msg, msg_len ) < 0 ){

        // comm error
        goto error;
    }

    // synchronize published vars that may have initializers
    fs_v_seek( f, sizeof(vm_size) + state.publish_start );

    for( uint8_t i = 0; i < state.publish_count; i++ ){

        vm_publish_t publish;

        fs_i16_read( f, (uint8_t *)&publish, sizeof(publish) );

        gfx_v_read_db_key( publish.hash );
    }   

    fs_f_close( f );

    // synchronize database parameters
    gfx_v_sync_db( TRUE );

    vm_sizes[vm_id] = vm_size_copy;

    log_v_debug_P( PSTR("VM loaded in: %lu ms"), tmr_u32_elapsed_time_ms( start_time ) );

    // init function is ready to run

    return 0;

error:
    
    fs_f_close( f );
    return -1;
}

#else

#endif

static bool is_vm_running( uint8_t vm_id ){

    return ( vm_status[vm_id] >= VM_STATUS_OK ) && ( vm_status[vm_id] != VM_STATUS_HALT );
}


PT_THREAD( vm_thread( pt_t *pt, vm_thread_state_t *state ) )
{
PT_BEGIN( pt );

    log_v_debug_P( PSTR("new thread %d on vm %d"), state->thread_id, state->vm_id );
    
    while( vm_active_threads[state->vm_id] & ( 1 << state->thread_id ) ){

        wifi_msg_vm_info_t info;
        if( _vm_i8_run_vm( state->vm_id, WIFI_DATA_ID_VM_RUN_THREAD, state->thread_id, &info ) == VM_STATUS_COMM_ERROR ){

            TMR_WAIT( pt, 100 );

            continue;
        }

        if( ( info.status < 0 ) || ( info.status == VM_STATUS_HALT ) ){

            vm_status[state->vm_id] = info.status;

            goto exit;
        }

        if( info.thread_delays[state->thread_id] < 0 ){

            // terminating thread
            goto exit;
        }

        int32_t delay = info.thread_delays[state->thread_id];
        if( delay < VM_MIN_DELAY ){

            delay = VM_MIN_DELAY;
        }


        TMR_WAIT( pt, delay );
    }

exit:
    vm_active_threads[state->vm_id] &= ~( 1 << state->thread_id );

    log_v_debug_P( PSTR("stop thread %d on vm %d"), state->thread_id, state->vm_id );

PT_END( pt );
}


PT_THREAD( vm_loader( pt_t *pt, void *state ) )
{
PT_BEGIN( pt );
    
    kvdb_v_set_name_P( PSTR("vm_0") );
    kvdb_v_set_name_P( PSTR("vm_1") );
    kvdb_v_set_name_P( PSTR("vm_2") );
    kvdb_v_set_name_P( PSTR("vm_3") );


    THREAD_WAIT_WHILE( pt, !wifi_b_attached() );


    while(1){

        
#ifdef VM_TARGET_ESP

        THREAD_WAIT_WHILE( pt, 
            ( ( ( !vm_run[0]  && !is_vm_running( 0 ) )  ||
                ( vm_run[0]   && is_vm_running( 0 ) ) )    &&
              ( !vm_reset[0] ) )
            &&
            ( ( ( !vm_run[1]  && !is_vm_running( 1 ) )  ||
                ( vm_run[1]   && is_vm_running( 1 ) ) )    &&
              ( !vm_reset[1] ) )
            &&
            ( ( ( !vm_run[2]  && !is_vm_running( 2 ) )  ||
                ( vm_run[2]   && is_vm_running( 2 ) ) )    &&
              ( !vm_reset[2] ) )
            &&
            ( ( ( !vm_run[3]  && !is_vm_running( 3 ) )  ||
                ( vm_run[3]   && is_vm_running( 3 ) ) )    &&
              ( !vm_reset[3] ) )
        );

        if( wifi_b_shutdown() ){

            THREAD_EXIT( pt );
        }


        // check what we're doing, and to what VM    
        for( uint8_t i = 0; i < VM_MAX_VMS; i++ ){

            // Was there an error and the VM is running
            if( ( vm_run[i] ) &&
                ( vm_status[i] != VM_STATUS_NOT_RUNNING ) &&
                ( vm_status[i] != VM_STATUS_READY ) &&
                ( vm_status[i] != 0 ) ){

                vm_run[i]               = FALSE;
                vm_reset[i]             = FALSE;
                vm_active_threads[i]    = 0;

                if( vm_status[i] == VM_STATUS_HALT ){

                    // this isn't actually an error, it is the VM
                    // signalling the script has requested a stop.
                    log_v_debug_P( PSTR("VM %d halted"), i );
                }
                else{

                    log_v_debug_P( PSTR("VM %d error: %d"), i, vm_status[i] );
                }
            }

            // Are we resetting a VM?
            if( vm_reset[i] ){

                log_v_debug_P( PSTR("Resetting VM: %d"), i );

                vm_status[i]            = VM_STATUS_NOT_RUNNING;
                vm_active_threads[i]    = 0;

                send_reset_message( i );
                reset_published_data( i );
            }

            // Did VM that was not running just get told to start?
            // This will also occur if we've triggered a reset
            if( vm_run[i] && !is_vm_running( i ) ){

                int8_t status = load_vm_wifi( i );

                if( status == 0 ){

                    wifi_msg_vm_info_t info;

                    // initialize VM (run init function)
                    vm_status[i] = _vm_i8_run_vm( i, WIFI_DATA_ID_INIT_VM, 0, &info );

                    if( vm_status[i] < 0 ){
                        
                        vm_run[i] = FALSE;

                        log_v_debug_P( PSTR("VM load fail: %d err: %d"), i, vm_status[i] );

                        goto error;
                    }

                    log_v_debug_P( PSTR("VM init %d OK: %d"), i, vm_status[i] );
                }
            }
            // Did VM that was running just get told to stop?
            else if( !vm_run[i] && is_vm_running( i ) ){

                log_v_debug_P( PSTR("Stopping VM: %d"), i );
                send_reset_message( i );
                reset_published_data( i );
                vm_cron_v_unload( i );
                vm_status[i] = VM_STATUS_NOT_RUNNING;

                vm_loop_time[i]         = 0;
                vm_thread_time[i]       = 0;
                vm_max_cycles[i]        = 0;
                vm_sizes[i]             = 0;
                vm_active_threads[i]    = 0;
            }
            
            // always reset the reset
            vm_reset[i] = FALSE;
        }

        TMR_WAIT( pt, 100 );
        continue;


    error:

        // longish delay after error to prevent swamping CPU trying
        // to reload a bad file.
        TMR_WAIT( pt, 1000 );


#else
  
#endif

    }

PT_END( pt );
}

int8_t vm_i8_run_loops( void ){

     for( uint8_t i = 0; i < VM_MAX_VMS; i++ ){
                    
        if( vm_b_is_vm_running( i ) ){

            wifi_msg_vm_info_t info;

            if( _vm_i8_run_vm( i, WIFI_DATA_ID_RUN_VM, 0, &info ) == VM_STATUS_COMM_ERROR ){

                return VM_STATUS_COMM_ERROR;
            }

            vm_status[i]            = info.status;
            vm_loop_time[i]         = info.run_time;
            vm_max_cycles[i]        = info.max_cycles;
        }
    }

    return 0;
}


void vm_v_init( void ){

    if( sys_u8_get_mode() == SYS_MODE_SAFE ){

        return;
    }

    COMPILER_ASSERT( ( sizeof(vm_state_t) % 4 ) == 0 );

    memset( vm_status, VM_STATUS_NOT_RUNNING, sizeof(vm_status) );

    thread_t_create( vm_loader,
                 PSTR("vm_loader"),
                 0,
                 0 );

    vm_cron_v_init();
}

void vm_v_start( void ){

    vm_run[0] = TRUE;
}

void vm_v_stop( void ){

    vm_run[0] = FALSE;
}

void vm_v_reset( void ){

    vm_reset[0] = TRUE;
}

bool vm_b_running( void ){

    for( uint8_t i = 0; i < VM_MAX_VMS; i++ ){

        if( is_vm_running( i ) ){

            return TRUE;
        }
    }

    return FALSE;
}

bool vm_b_is_vm_running( uint8_t i ){

    ASSERT( i < VM_MAX_VMS );

    return is_vm_running( i );   
}
