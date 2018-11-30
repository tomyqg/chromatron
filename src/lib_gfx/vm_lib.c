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

#include <string.h>
#include <stdint.h>
#include "bool.h"
#include "catbus_common.h"
#include "random.h"

#include "vm_lib.h"

int8_t vm_lib_i8_libcall_built_in( 
	catbus_hash_t32 func_hash, 
    vm_state_t *state, 
	int32_t *data,
	int32_t *result, 
	int32_t *params, 
	uint16_t param_len ){

	int32_t temp0, temp1, array_len;

	switch( func_hash ){
        case __KV__rand:

            if( param_len == 0 ){

                *result = rnd_u16_get_int_with_seed( &state->rng_seed );

                break;
            }
            else if( param_len == 1 ){

                temp0 = data[params[0]];
                temp1 = 0;
            }
            else{

                temp0 = data[params[1]] - data[params[0]];
                temp1 = data[params[0]];
            }

            // check for divide by 0, or negative spread
            if( temp0 <= 0 ){

                // just return the offset
                *result = temp1;
                
            }
            else{
            
                *result = temp1 + ( rnd_u16_get_int_with_seed( &state->rng_seed ) % temp0 );
            }

            break;

		case __KV__test_lib_call:
            if( param_len != 2 ){

                return 0;
            }

            *result = data[params[0]] + data[params[1]];
            break;

      	case __KV__min:
            if( param_len != 2 ){

                return 0;
            }

            array_len = data[params[1]];

            temp0 = data[params[0]];

            // second param is array len
            for( int32_t i = 1; i < array_len; i++ ){

            	temp1 = data[params[0] + i];

            	if( temp1 < temp0 ){

            		temp0 = temp1;
            	}
            }

            *result = temp0;
            break;

        case __KV__max:
            if( param_len != 2 ){

                return 0;
            }

            array_len = data[params[1]];

            temp0 = data[params[0]];

            // second param is array len
            for( int32_t i = 1; i < array_len; i++ ){

                temp1 = data[params[0] + i];

                if( temp1 > temp0 ){

                    temp0 = temp1;
                }
            }

            *result = temp0;
            break;

        case __KV__sum:
            if( param_len != 2 ){

                return 0;
            }

            array_len = data[params[1]];

            // second param is array len
            for( int32_t i = 0; i < array_len; i++ ){

                *result += data[params[0] + i];
            }

            break;

        case __KV__avg:
            if( param_len != 2 ){

                return 0;
            }

            // check for divide by zero
            if( params[0] == 0 ){

                return 0;
            }

            array_len = data[params[1]];

            // second param is array len
            for( int32_t i = 0; i < array_len; i++ ){

                *result += data[params[0] + i];
            }

            *result /= array_len;

            break;

        case __KV__yield:

            state->yield = TRUE;
            break;

        case __KV__delay:
            // default to yield (no delay) if no parameters
            if( param_len < 1 ){

                temp0 = 0;
            }
            else{
                // first parameter is delay time, all other params ignored
                temp0 = data[params[0]];
            }

            // set up delay
            state->threads[state->current_thread].delay_ticks = temp0 / state->tick_rate;

            // delay also yields
            state->yield = TRUE;
            break;

        case __KV__start_thread:
            if( param_len != 1 ){

                return 0;
            }

            // params[0] - thread addr

            // search for an empty slot
            for( uint8_t i = 0; i < cnt_of_array(state->threads); i++ ){

                if( state->threads[i].func_addr == 0xffff ){

                    memset( &state->threads[i], 0, sizeof(state->threads[i]) );

                    state->threads[i].func_addr = params[0];

                    break;
                }
            }

            break;

        case __KV__stop_thread:
            if( param_len != 1 ){

                return 0;
            }

            // params[0] - thread addr

            // search for matching threads
            for( uint8_t i = 0; i < cnt_of_array(state->threads); i++ ){

                if( state->threads[i].func_addr == params[0] ){

                    state->threads[i].func_addr = 0xffff;

                    break;
                }
            }

            break;

        case __KV__thread_running:
            if( param_len != 1 ){

                return 0;
            }

            // params[0] - thread addr

            // search for matching threads
            for( uint8_t i = 0; i < cnt_of_array(state->threads); i++ ){

                if( state->threads[i].func_addr == params[0] ){

                    return TRUE;
                }
            }

            break;

		default:
            // function not found
            return -1;
            break;
    }    
	
    return 0;
}
