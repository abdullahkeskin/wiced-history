/**
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
#include "wiced.h"
#include "wiced_bt_smartbridge.h"

#include "wiced_bt_gatt.h"
#include "wiced_bt_ble.h"
#include "wiced_bt_cfg.h"

#include "bt_smart_gatt.h"

#include "bt_transport_thread.h"
#include "bt_smartbridge_socket_manager.h"
#include "bt_smartbridge_att_cache_manager.h"
#include "smartbridge_helper.h"
#include "smartbridge_stack_if.h"

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

static wiced_bt_smart_scan_result_t*                scan_result_head        = NULL;
static wiced_bt_smart_scan_result_t*                scan_result_tail        = NULL;
static uint32_t                                     scan_result_count       = 0;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t smartbridge_helper_get_scan_results( wiced_bt_smart_scan_result_t** result_list, uint32_t* count )
{
    if ( smartbridge_bt_interface_is_scanning() == WICED_TRUE )
    {
        WPRINT_LIB_INFO(("[Smartbridge] Can't Fetch Scan-Results [ Scan in-progress ? ]\n"));
        return WICED_BT_SCAN_IN_PROGRESS;
    }

    *result_list = scan_result_head;
    *count       = scan_result_count;
    return WICED_BT_SUCCESS;
}

wiced_result_t smartbridge_helper_delete_scan_result_list( void )
{
    wiced_bt_smart_scan_result_t* curr;

    if ( scan_result_count == 0 )
    {
        return WICED_BT_LIST_EMPTY;
    }

    curr = scan_result_head;

    /* Traverse through the list and delete all attributes */
    while ( curr != NULL )
    {
        /* Store pointer to next because curr is about to be deleted */
        wiced_bt_smart_scan_result_t* next = curr->next;

        /* Detach result from the list and free memory */
        curr->next = NULL;
        free( curr );

        /* Update curr */
        curr = next;
    }

    scan_result_count = 0;
    scan_result_head  = NULL;
    scan_result_tail  = NULL;
    return WICED_BT_SUCCESS;
}

wiced_result_t smartbridge_helper_add_scan_result_to_list( wiced_bt_smart_scan_result_t* result )
{
    if ( scan_result_count == 0 )
    {
        scan_result_head = result;
        scan_result_tail = result;
    }
    else
    {
        scan_result_tail->next = result;
        scan_result_tail       = result;
    }

    scan_result_count++;
    WPRINT_LIB_INFO(("[SmartBridge]New scan-result-count:%d\n", (int)scan_result_count));
    result->next = NULL;

    return WICED_BT_SUCCESS;
}

wiced_result_t smartbridge_helper_find_device_in_scan_result_list( wiced_bt_device_address_t* address, wiced_bt_smart_address_type_t type,  wiced_bt_smart_scan_result_t** result )
{
    wiced_bt_smart_scan_result_t* iterator = scan_result_head;

    while( iterator != NULL )
    {
        if ( ( memcmp( &iterator->remote_device.address, address, sizeof( *address ) ) == 0 ) && ( iterator->remote_device.address_type == type ) )
        {
            *result = iterator;
            return WICED_BT_SUCCESS;
        }

        iterator = iterator->next;
    }

    return WICED_BT_ITEM_NOT_IN_LIST;
}

/******************************************************
 *            Socket Action Helper Functions
 ******************************************************/

wiced_bool_t smartbridge_helper_socket_check_actions_enabled( wiced_bt_smartbridge_socket_t* socket, uint8_t action_bits )
{
    return ( ( socket->actions & action_bits ) == action_bits ) ? WICED_TRUE : WICED_FALSE;
}

wiced_bool_t smartbridge_helper_socket_check_actions_disabled( wiced_bt_smartbridge_socket_t* socket, uint8_t action_bits )
{
    return ( ( socket->actions | ~action_bits ) == ~action_bits ) ? WICED_TRUE : WICED_FALSE;
}

void smartbridge_helper_socket_set_actions( wiced_bt_smartbridge_socket_t* socket, uint8_t action_bits )
{
    socket->actions |= action_bits;
}

void smartbridge_helper_socket_clear_actions( wiced_bt_smartbridge_socket_t* socket, uint8_t action_bits )
{
    socket->actions &= ~action_bits;
}