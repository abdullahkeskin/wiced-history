/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 */
#include "besl_host_interface.h"
#include "wwd_structures.h"
#include "wiced_utilities.h"
#include "wwd_wifi.h"
#include "wwd_crypto.h"
#include "internal/SDPCM.h"
#include "besl_host.h"
#include "internal/bcmendian.h"
#include <string.h>
#include "Network/wwd_buffer_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

void* besl_host_malloc( char* name, uint32_t size )
{
    return malloc_named( name, size );
}

void besl_host_free( void* p )
{
    free( p );
}

void besl_host_get_mac_address(besl_mac_t* address, uint32_t interface )
{
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    wiced_result_t result;
    uint32_t*      data;

    data = wiced_get_iovar_buffer( &buffer, sizeof(wiced_mac_t) + sizeof(uint32_t), "bsscfg:" IOVAR_STR_CUR_ETHERADDR );
    if ( data == NULL )
    {
        return;
    }
    *data = interface;

    result = wiced_send_iovar( SDPCM_GET, buffer, &response, SDPCM_STA_INTERFACE );
    if ( result == WICED_SUCCESS )
    {
        memcpy( address, host_buffer_get_current_piece_data_pointer( response ), sizeof(wiced_mac_t) );
        host_buffer_release( response, WICED_NETWORK_RX );
    }
}

void besl_host_set_mac_address(besl_mac_t* address, uint32_t interface )
{
    wiced_buffer_t buffer;
    uint32_t*      data;

    data = wiced_get_iovar_buffer( &buffer, sizeof(wiced_mac_t) + sizeof(uint32_t), "bsscfg:" IOVAR_STR_CUR_ETHERADDR );
    if ( data == NULL )
    {
        return;
    }
    data[0] = interface;
    memcpy(&data[1], address, sizeof(wiced_mac_t));

    wiced_send_iovar( SDPCM_SET, buffer, NULL, SDPCM_STA_INTERFACE );
}


wiced_result_t wiced_besl_wifi_get_random( uint16_t* val )
{
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    wiced_result_t ret;
    static uint16_t pseudo_random = 0;

    (void) wiced_get_iovar_buffer( &buffer, (uint16_t) 2, IOVAR_STR_RAND ); /* Do not need to put anything in buffer hence void cast */
    ret = wiced_send_iovar( SDPCM_GET, buffer, &response, SDPCM_STA_INTERFACE );

    if ( ret == WICED_SUCCESS )
    {
        uint8_t* data = (uint8_t*) host_buffer_get_current_piece_data_pointer( response );
        memcpy( val, data, (size_t) 2 );
        host_buffer_release( response, WICED_NETWORK_RX );
    }
    else
    {
        // Use a pseudo random number
        if ( pseudo_random == 0 )
        {
            pseudo_random = (uint16_t) host_rtos_get_time( );
        }
        pseudo_random = (uint16_t) ( ( pseudo_random * 32719 + 3 ) % 32749 );

        *val = pseudo_random;
    }

    return WICED_SUCCESS;
}


void besl_host_random_bytes(uint8_t* buffer, uint16_t buffer_length)
{
    for ( ; buffer_length != 0; buffer_length -= 2 )
    {
        uint16_t temp_random;
        if ( wiced_besl_wifi_get_random( &temp_random ) != WICED_SUCCESS )
        {
            int32_t temp = 0xA5;
            wiced_wifi_get_rssi( &temp );
            BESL_WRITE_16(buffer, temp);
        }
        else
        {
            BESL_WRITE_16(buffer, temp_random);
        }
        buffer = buffer + 2;
    }
}


uint32_t besl_host_hton32(uint32_t intlong)
{
    return htobe32(intlong);
}

uint16_t besl_host_htol16(uint16_t intshort)
{
    return intshort;
}

uint16_t besl_host_hton16_ptr(uint8_t * in, uint8_t * out)
{
    uint16_t temp;
    temp = BESL_READ_16(in);
    temp = htobe16(temp);
    BESL_WRITE_16(out, temp);
    return temp;
}

uint16_t besl_host_hton16(uint16_t intshort)
{
    return htobe16(intshort);
}

uint16_t besl_host_ltoh16(uint16_t intshort)
{
	return ntoh16(intshort);
}

uint32_t besl_host_hton32_ptr(uint8_t * in, uint8_t * out)
{
    uint32_t temp;
    temp = BESL_READ_32(in);
    temp = htobe32(temp);
    BESL_WRITE_32(out, temp);
    return temp;
}
