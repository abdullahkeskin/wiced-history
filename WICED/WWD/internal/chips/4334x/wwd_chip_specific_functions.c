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
#include "internal/wwd_internal.h"
#include "internal/bus_protocols/wwd_bus_protocol_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define VERIFY_RESULT( x )     { wwd_result_t verify_result; verify_result = ( x ); if ( verify_result != WWD_SUCCESS ) return verify_result; }

/******************************************************
 *                    Constants
 ******************************************************/

#define WLAN_BUS_UP_ATTEMPTS    ( 1000 )
#define KSO_WAIT_MS             ( 1 )
#define MAX_KSO_ATTEMPTS        ( 64 )

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

#ifndef WWD_DISABLE_SAVE_RESTORE
static wwd_result_t wwd_enable_save_restore( void );
#endif

static wwd_result_t wwd_kso_enable( wiced_bool_t enable );

/******************************************************
 *               Variables Definitions
 ******************************************************/

static wiced_bool_t bus_is_up               = WICED_FALSE;
static wiced_bool_t save_restore_enable     = WICED_FALSE;

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
    UNUSED_PARAMETER(country_code);
    return WWD_UNSUPPORTED;
}

wwd_result_t wwd_chip_specific_init( void )
{
#ifndef WWD_DISABLE_SAVE_RESTORE
    return wwd_enable_save_restore();
#else
    return WWD_SUCCESS;
#endif
}

wwd_result_t wwd_disable_sram3_remap( void )
{
    return WWD_SUCCESS;
}

wwd_result_t wwd_ensure_wlan_bus_is_up( void )
{
    /* Ensure HT clock is up */
    if ( bus_is_up == WICED_TRUE )
    {
        return WWD_SUCCESS;
    }

    if ( save_restore_enable == WICED_FALSE )
    {
        uint8_t csr = 0;
        uint32_t attempts = (uint32_t) WLAN_BUS_UP_ATTEMPTS;

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
    else
    {
        if ( wwd_kso_enable( WICED_TRUE ) == WWD_SUCCESS )
        {
            bus_is_up = WICED_TRUE;
            return WWD_SUCCESS;
        }
        else
        {
            return WWD_SDIO_BUS_UP_FAIL;
        }
    }
}

wwd_result_t wwd_allow_wlan_bus_to_sleep( void )
{
    /* Clear HT clock request */
    if ( bus_is_up == WICED_TRUE )
    {
        bus_is_up = WICED_FALSE;
        if ( save_restore_enable == WICED_FALSE )
        {
            return wwd_bus_write_register_value( BACKPLANE_FUNCTION, (uint32_t) SDIO_CHIP_CLOCK_CSR, (uint8_t) 1, 0 );
        }
        else
        {
           return wwd_kso_enable( WICED_FALSE );
        }
    }
    else
    {
        return WWD_SUCCESS;
    }
}

#ifndef WWD_DISABLE_SAVE_RESTORE
static wwd_result_t wwd_enable_save_restore( void )
{
    uint32_t core_capext;
    uint32_t retention_ctl;
    uint8_t  data;
    wiced_bool_t save_restore_capable = WICED_FALSE;

    /* check if fw initialized sr engine */
    VERIFY_RESULT( wwd_bus_read_backplane_value( (uint32_t) CHIPCOMMON_CORE_CAPEXT_ADDR, (uint8_t) 4, (uint8_t*)&core_capext ));

    if (( core_capext & CHIPCOMMON_CORE_CAPEXT_SR_SUPPORTED ) != 0 )
    {
        VERIFY_RESULT( wwd_bus_read_backplane_value( (uint32_t) CHIPCOMMON_CORE_RETENTION_CTL, (uint8_t) 4, (uint8_t*)&retention_ctl ));
        if (( retention_ctl & ( CHIPCOMMON_CORE_RCTL_MACPHY_DISABLE | CHIPCOMMON_CORE_RCTL_LOGIC_DISABLE )) == 0 )
        {
            save_restore_capable = WICED_TRUE;
        }
    }

    if ( save_restore_capable == WICED_TRUE )
    {
        /* Configure WakeupCtrl register to set HtAvail request bit in chipClockCSR register
         * after the sdiod core is powered on.
         */
        VERIFY_RESULT( wwd_bus_read_register_value( BACKPLANE_FUNCTION, (uint32_t ) SDIO_WAKEUP_CTRL, (uint8_t ) 1, &data ) );
        data |= SBSDIO_WCTRL_WAKE_TILL_HT_AVAIL;
        VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, (uint32_t ) SDIO_WAKEUP_CTRL, (uint8_t ) 1, data ) );

        /* Set brcmCardCapability to noCmdDecode mode.
         * It makes sdiod_aos to wakeup host for any activity of cmd line, even though
         * module won't decode cmd or respond
         */
        VERIFY_RESULT( wwd_bus_write_register_value( BUS_FUNCTION, (uint32_t ) SDIOD_CCCR_BRCM_CARDCAP, (uint8_t ) 1, SDIOD_CCCR_BRCM_CARDCAP_CMD_NODEC ) );

        VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, (uint32_t) SDIO_CHIP_CLOCK_CSR, (uint8_t) 1, (uint32_t) SBSDIO_FORCE_HT ) );

        /* Enable KeepSdioOn (KSO) bit for normal operation */
        VERIFY_RESULT( wwd_bus_read_register_value( BACKPLANE_FUNCTION, (uint32_t ) SDIO_SLEEP_CSR, (uint8_t ) 1, &data ) );
        if ( ( data & SBSDIO_SLPCSR_KEEP_SDIO_ON ) == 0 )
        {
            data |= SBSDIO_SLPCSR_KEEP_SDIO_ON;
            VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, (uint32_t ) SDIO_SLEEP_CSR, (uint8_t ) 1, data ) );
        }

        /* SPI bus can be configured for sleep by default.
         * KSO bit solely controls the wlan chip sleep
         */
        VERIFY_RESULT( wwd_bus_specific_sleep( ));

#ifdef WWD_SPI_IRQ_FALLING_EDGE
        /* Put SPI interface block to sleep */
        VERIFY_RESULT( wwd_bus_write_register_value( BACKPLANE_FUNCTION, SDIO_PULL_UP,  (uint8_t) 1, 0xf ));
#endif /* WWD_SPI_IRQ_FALLING_EDGE */

        save_restore_enable = WICED_TRUE;
    }
    else
    {
        save_restore_enable = WICED_FALSE;
    }

    return WWD_SUCCESS;
}
#endif /* #ifndef WWD_DISABLE_SAVE_RESTORE */

static wwd_result_t wwd_kso_enable (wiced_bool_t enable)
{
    uint8_t write_value = 0;
    uint8_t read_value = 0;
    uint8_t compare_value;
    uint8_t bmask;
    uint32_t attempts = ( uint32_t ) MAX_KSO_ATTEMPTS;
    wwd_result_t result;

    if ( enable == WICED_TRUE )
    {
        write_value |= SBSDIO_SLPCSR_KEEP_SDIO_ON;
    }

    /* 1st KSO write goes to AOS wake up core if device is asleep  */
    /* Possibly device might not respond to this cmd. So, don't check return value here */
    /* 2 Sequential writes to KSO bit are required for SR module to wakeup */
    wwd_bus_write_register_value( BACKPLANE_FUNCTION, (uint32_t) SDIO_SLEEP_CSR, (uint8_t) 1, write_value );
    wwd_bus_write_register_value( BACKPLANE_FUNCTION, (uint32_t) SDIO_SLEEP_CSR, (uint8_t) 1, write_value );

    if ( enable == WICED_TRUE )
    {
        /* device WAKEUP through KSO:
         * write bit 0 & read back until
         * both bits 0(kso bit) & 1 (dev on status) are set
         */
        compare_value = SBSDIO_SLPCSR_KEEP_SDIO_ON | SBSDIO_SLPCSR_DEVICE_ON;
        bmask = compare_value;

        host_rtos_delay_milliseconds( (uint32_t) 3 );
    }
    else
    {
        /* Put device to sleep, turn off  KSO  */
        compare_value = 0;
        /* Check for bit0 only, bit1(devon status) may not get cleared right away */
        bmask = SBSDIO_SLPCSR_KEEP_SDIO_ON;
    }

    do
    {
        /* Reliable KSO bit set/clr:
         * Sdiod sleep write access appears to be in sync with PMU 32khz clk
         * just one write attempt may fail,(same is with read ?)
         * in any case, read it back until it matches written value
         */
        result = wwd_bus_read_register_value( BACKPLANE_FUNCTION, (uint32_t) SDIO_SLEEP_CSR, (uint8_t) 1, &read_value );
        if ( ( ( read_value & bmask ) == compare_value ) && ( result == WWD_SUCCESS ) )
        {
            break;
        }

        host_rtos_delay_milliseconds( (uint32_t) KSO_WAIT_MS );

        wwd_bus_write_register_value( BACKPLANE_FUNCTION, (uint32_t) SDIO_SLEEP_CSR, (uint8_t) 1, write_value );
        attempts--;
    } while ( attempts != 0 );

    if ( attempts == 0 )
    {
        return WWD_SDIO_BUS_UP_FAIL;
    }
    else
    {
        return WWD_SUCCESS;
    }
}