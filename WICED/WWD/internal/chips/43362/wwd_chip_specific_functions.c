/*
 * Copyright 2015, Broadcom Corporation
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
#include "wwd_constants.h"
#include "wwd_wifi.h"
#include "internal/wwd_sdpcm.h"
#include <string.h>
#include "internal/wwd_internal.h"
#include "internal/bus_protocols/wwd_bus_protocol_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define VERIFY_RESULT( x )     { wwd_result_t verify_result; verify_result = ( x ); if ( verify_result != WWD_SUCCESS ) return verify_result; }

/******************************************************
 *                    Constants
 ******************************************************/
#define WLAN_BUS_UP_ATTEMPTS  (1000)

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
static wiced_bool_t bus_is_up               = WICED_FALSE;

/******************************************************
 *               Function Definitions
 ******************************************************/

wwd_result_t wwd_wifi_read_wlan_log( char* buffer, uint32_t buffer_size )
{
    UNUSED_PARAMETER(buffer);
    UNUSED_PARAMETER(buffer_size);
    return WWD_UNSUPPORTED;
}

wwd_result_t wwd_wifi_set_custom_country_code( const wiced_country_info_t* country_code )
{
    wiced_buffer_t buffer;
    wwd_result_t   result;
    wiced_country_info_t* data;

    data = (wiced_country_info_t*) wwd_sdpcm_get_ioctl_buffer( &buffer, (uint16_t) sizeof(wiced_country_info_t) + 10 );
    memcpy( data, country_code, sizeof(wiced_country_info_t) );
    result = wwd_sdpcm_send_ioctl( SDPCM_SET, WLC_SET_CUSTOM_COUNTRY, buffer, NULL, WWD_STA_INTERFACE );

    return result;
}

wwd_result_t wwd_chip_specific_init( void )
{
    return WWD_SUCCESS;
}

wwd_result_t wwd_disable_sram3_remap( void )
{
    return WWD_SUCCESS;
}

wwd_result_t wwd_ensure_wlan_bus_is_up( void )
{
    uint8_t csr = 0;
    uint32_t attempts = (uint32_t) WLAN_BUS_UP_ATTEMPTS;

    /* Ensure HT clock is up */
    if ( bus_is_up == WICED_TRUE )
    {
        return WWD_SUCCESS;
    }

    /* Bus specific wakeup routine */
    VERIFY_RESULT( wwd_bus_specific_wakeup( ));

    VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, (uint32_t) SDIO_CHIP_CLOCK_CSR, (uint8_t) 1, (uint32_t) SBSDIO_HT_AVAIL_REQ ) );

    do
    {
        VERIFY_RESULT( wwd_bus_read_register_value( BACKPLANE_FUNCTION, (uint32_t) SDIO_CHIP_CLOCK_CSR, (uint8_t) 1, &csr ) );
        --attempts;
    }
    while ( ( ( csr & SBSDIO_HT_AVAIL ) == 0 ) &&
            ( attempts != 0 ) &&
            ( host_rtos_delay_milliseconds( (uint32_t) 1 ), 1==1 ) );

    if (attempts == 0)
    {
        return WWD_SDIO_BUS_UP_FAIL;
    }
    else
    {
        bus_is_up = WICED_TRUE;
        return WWD_SUCCESS;
    }
}

wwd_result_t wwd_allow_wlan_bus_to_sleep( void )
{
    /* Clear HT clock request */
    if (bus_is_up == WICED_TRUE)
    {
        bus_is_up = WICED_FALSE;
        VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, (uint32_t) SDIO_CHIP_CLOCK_CSR, (uint8_t) 1, 0 ));

        /* Bus specific sleep routine */
        return wwd_bus_specific_sleep( );
    }
    else
    {
        return WWD_SUCCESS;
    }
}
