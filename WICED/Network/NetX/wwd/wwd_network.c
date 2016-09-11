/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/******************************************************************************
 This driver was constructed using the NetX User Guide as a reference.
 It is available at http://www.rtos.com/PDFs/NetX_User_Guide.pdf
 *****************************************************************************/

#include "nx_api.h"
#include "wwd_wifi.h"
#include "Network/wwd_network_interface.h"
#include "wwd_network.h"
#include "Network/wwd_network_constants.h"
#include "wwd_assert.h"

#if ( NX_PHYSICAL_HEADER != WICED_PHYSICAL_HEADER )
#error ERROR PHYSICAL HEADER SIZE CHANGED - PREBUILT NETX-DUO LIBRARY WILL NOT WORK
#endif

#if ( NX_PHYSICAL_TRAILER != WICED_PHYSICAL_TRAILER )
#error ERROR PHYSICAL TRAILER SIZE CHANGED - PREBUILT NETX-DUO LIBRARY WILL NOT WORK
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define IP_HANDLE(x)    (ip_ptr[(x)&1])

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

static void nx_wiced_add_ethernet_header( NX_IP* ip_ptr, NX_PACKET* packet_ptr, ULONG destination_mac_msw, ULONG destination_mac_lsw, USHORT ethertype );
static VOID wiced_netx_driver_entry( NX_IP_DRIVER* driver, wiced_interface_t interface );

#ifdef ADD_NETX_EAPOL_SUPPORT
/*@external@*/ extern void host_network_process_eapol_data( wiced_buffer_t buffer, wiced_interface_t interface );
#endif

/******************************************************
 *               Variables Definitions
 ******************************************************/

/* Saves pointers to the IP instances so that the receive function knows where to send data */
static NX_IP* ip_ptr[2] = { NULL, NULL };

/******************************************************
 *               Function Definitions
 ******************************************************/

VOID wiced_sta_netx_driver_entry( NX_IP_DRIVER* driver )
{
    wiced_netx_driver_entry( driver, WICED_STA_INTERFACE );
}

VOID wiced_ap_netx_driver_entry( NX_IP_DRIVER* driver )
{
    wiced_netx_driver_entry( driver, WICED_AP_INTERFACE );
}

static VOID wiced_netx_driver_entry( NX_IP_DRIVER* driver, wiced_interface_t interface )
{
    NX_PACKET*  packet_ptr;
    wiced_mac_t mac;

    wiced_assert("Bad args", driver != NULL);

    packet_ptr = driver->nx_ip_driver_packet;

    /* Save the IP instance pointer so that the receive function will know where to send data */
    driver->nx_ip_driver_status = NX_NOT_SUCCESSFUL;

    /* Process commands which are valid independent of the link state */
    switch ( driver->nx_ip_driver_command )
    {
        case NX_LINK_INITIALIZE:
            IP_HANDLE(interface) = driver->nx_ip_driver_ptr;
            IP_HANDLE(interface)->nx_ip_driver_mtu            = (ULONG) ( WICED_LINK_MTU - NX_PHYSICAL_HEADER - NX_PHYSICAL_TRAILER );
            IP_HANDLE(interface)->nx_ip_driver_mapping_needed = (UINT) NX_TRUE;
            driver->nx_ip_driver_status                       = (UINT) NX_SUCCESS;
            break;

        case NX_LINK_UNINITIALIZE:
            IP_HANDLE(interface) = NULL;
            break;

        case NX_LINK_ENABLE:
            if ( wiced_wifi_get_mac_address( &mac ) != WICED_SUCCESS )
            {
                IP_HANDLE(interface)->nx_ip_driver_link_up = NX_FALSE;
                break;
            }

            IP_HANDLE(interface)->nx_ip_arp_physical_address_msw = (ULONG) ( ( mac.octet[0] << 8 ) + mac.octet[1] );
            IP_HANDLE(interface)->nx_ip_arp_physical_address_lsw = (ULONG) ( ( mac.octet[2] << 24 ) + ( mac.octet[3] << 16 ) + ( mac.octet[4] << 8 ) + mac.octet[5] );

            IP_HANDLE(interface)->nx_ip_driver_link_up = (UINT) NX_TRUE;
            driver->nx_ip_driver_status = (UINT) NX_SUCCESS;
            break;

        case NX_LINK_DISABLE:
            IP_HANDLE(interface)->nx_ip_driver_link_up = NX_FALSE;
            driver->nx_ip_driver_status = (UINT) NX_SUCCESS;
            break;

        case NX_LINK_MULTICAST_JOIN:
            mac.octet[0] = (uint8_t) ( ( driver->nx_ip_driver_physical_address_msw & 0x0000ff00 ) >> 8 );
            mac.octet[1] = (uint8_t) ( ( driver->nx_ip_driver_physical_address_msw & 0x000000ff ) >> 0 );
            mac.octet[2] = (uint8_t) ( ( driver->nx_ip_driver_physical_address_lsw & 0xff000000 ) >> 24 );
            mac.octet[3] = (uint8_t) ( ( driver->nx_ip_driver_physical_address_lsw & 0x00ff0000 ) >> 16 );
            mac.octet[4] = (uint8_t) ( ( driver->nx_ip_driver_physical_address_lsw & 0x0000ff00 ) >> 8 );
            mac.octet[5] = (uint8_t) ( ( driver->nx_ip_driver_physical_address_lsw & 0x000000ff ) >> 0 );

            if ( wiced_wifi_register_multicast_address( &mac ) != WICED_SUCCESS )
            {
                driver->nx_ip_driver_status = (UINT) NX_NOT_SUCCESSFUL;
            }
            driver->nx_ip_driver_status = (UINT) NX_SUCCESS;
            break;

        case NX_LINK_MULTICAST_LEAVE:
            mac.octet[0] = 0;
            mac.octet[1] = 0;
            mac.octet[2] = 0;
            mac.octet[3] = 0;
            mac.octet[4] = 0;
            mac.octet[5] = 0;

            if ( wiced_wifi_unregister_multicast_address( &mac ) != WICED_SUCCESS )
            {
                driver->nx_ip_driver_status = (UINT) NX_NOT_SUCCESSFUL;
            }
            driver->nx_ip_driver_status = (UINT) NX_SUCCESS;
            break;

        case NX_LINK_GET_STATUS:
            /* Signal status through return pointer */
            *(driver -> nx_ip_driver_return_ptr) = (ULONG) IP_HANDLE(interface)->nx_ip_driver_link_up;
            driver->nx_ip_driver_status = (UINT) NX_SUCCESS;
            break;

        case NX_LINK_PACKET_SEND:
        case NX_LINK_ARP_RESPONSE_SEND:
        case NX_LINK_ARP_SEND:
        case NX_LINK_RARP_SEND:
        case NX_LINK_PACKET_BROADCAST:
            /* These cases require the link to be up, and will be processed below if it is up. */
            break;

        case NX_LINK_DEFERRED_PROCESSING:
        default:
            /* Invalid driver request */
            driver->nx_ip_driver_status = (UINT) NX_UNHANDLED_COMMAND;
            break;
    }

    /* Check if the link is up */
    if ( ( IP_HANDLE(interface)->nx_ip_driver_link_up == NX_TRUE ) && ( wiced_wifi_is_ready_to_transceive( interface ) == WICED_SUCCESS ) )
    {
        switch ( driver->nx_ip_driver_command )
        {
            case NX_LINK_PACKET_SEND:
                nx_wiced_add_ethernet_header( IP_HANDLE(interface), packet_ptr, driver->nx_ip_driver_physical_address_msw, driver->nx_ip_driver_physical_address_lsw, (USHORT) WICED_ETHERTYPE_IPv4 );
                wiced_network_send_ethernet_data( (wiced_buffer_t) packet_ptr, interface );
                driver->nx_ip_driver_status = (UINT) NX_SUCCESS;
                break;

            case NX_LINK_ARP_RESPONSE_SEND:
                nx_wiced_add_ethernet_header( IP_HANDLE(interface), packet_ptr, driver->nx_ip_driver_physical_address_msw, driver->nx_ip_driver_physical_address_lsw, (USHORT) WICED_ETHERTYPE_ARP );
                wiced_network_send_ethernet_data( (wiced_buffer_t) packet_ptr, interface );
                driver->nx_ip_driver_status = (UINT) NX_SUCCESS;
                break;

            case NX_LINK_ARP_SEND:
                nx_wiced_add_ethernet_header( IP_HANDLE(interface), packet_ptr, (ULONG) 0xFFFF, (ULONG) 0xFFFFFFFF, (USHORT) WICED_ETHERTYPE_ARP );
                wiced_network_send_ethernet_data( (wiced_buffer_t) packet_ptr, interface );
                driver->nx_ip_driver_status = (UINT) NX_SUCCESS;
                break;

            case NX_LINK_RARP_SEND:
                nx_wiced_add_ethernet_header( IP_HANDLE(interface), packet_ptr, (ULONG) 0xFFFF, (ULONG) 0xFFFFFFFF, (USHORT) WICED_ETHERTYPE_RARP );
                wiced_network_send_ethernet_data( (wiced_buffer_t) packet_ptr, interface );
                driver->nx_ip_driver_status = (UINT) NX_SUCCESS;
                break;

            case NX_LINK_PACKET_BROADCAST:
                nx_wiced_add_ethernet_header( IP_HANDLE(interface), packet_ptr, (ULONG) 0xFFFF, (ULONG) 0xFFFFFFFF, (USHORT) WICED_ETHERTYPE_IPv4 );
                wiced_network_send_ethernet_data( (wiced_buffer_t) packet_ptr, interface );
                driver->nx_ip_driver_status = (UINT) NX_SUCCESS;
                break;

            case NX_LINK_INITIALIZE:
            case NX_LINK_ENABLE:
            case NX_LINK_DISABLE:
            case NX_LINK_MULTICAST_JOIN:
            case NX_LINK_MULTICAST_LEAVE:
            case NX_LINK_GET_STATUS:
            case NX_LINK_DEFERRED_PROCESSING:
            default:
                /* Handled in above case statement */
                break;
        }
    }
    else
    {
        /* Link is down, free any packet provided by the command */
        if ( packet_ptr != NULL )
        {
            switch (driver->nx_ip_driver_command)
            {
                case NX_LINK_PACKET_BROADCAST:
                case NX_LINK_RARP_SEND:
                case NX_LINK_ARP_SEND:
                case NX_LINK_ARP_RESPONSE_SEND:
                case NX_LINK_PACKET_SEND:
                    nx_packet_release(packet_ptr);
                    break;

                default:
                    break;
            }
        }
    }
}

void host_network_process_ethernet_data( wiced_buffer_t buffer, wiced_interface_t interface )
{
    USHORT         ethertype;
    NX_PACKET*     packet_ptr = (NX_PACKET*) buffer;
    unsigned char* buff       = packet_ptr->nx_packet_prepend_ptr;

    if ( buff == NULL )
    {
        return;
    }

    ethertype = (USHORT)(buff[12] << 8 | buff[13]);

#ifdef ADD_NETX_EAPOL_SUPPORT
    if ( ethertype == WICED_ETHERTYPE_EAPOL )
    {
         /* pass it to the WPS interface, but do not release the packet */
         host_network_process_eapol_data(packet_ptr, interface);
    }
    else
#endif
    {
        /* Check if interface is not attached to IP instance, if so drop frame */
        if ( IP_HANDLE(interface) == NULL )
        {
            nx_packet_release(packet_ptr);
            return;
        }

        /* Remove the ethernet header, so packet is ready for reading by NetX */
        packet_ptr->nx_packet_prepend_ptr = packet_ptr->nx_packet_prepend_ptr + WICED_ETHERNET_SIZE;
        packet_ptr->nx_packet_length      = packet_ptr->nx_packet_length - WICED_ETHERNET_SIZE;

        if ( ( ethertype == WICED_ETHERTYPE_IPv4 ) || ethertype == WICED_ETHERTYPE_IPv6 )
        {
#ifdef NX_DIRECT_ISR_CALL
            _nx_ip_packet_receive(ip_ptr, packet_ptr);
#else
            _nx_ip_packet_deferred_receive( ip_ptr[(int)interface], packet_ptr );
#endif
        }
        else if ( ethertype == WICED_ETHERTYPE_ARP )
        {
            _nx_arp_packet_deferred_receive( ip_ptr[(int)interface], packet_ptr );
        }
        else if ( ethertype == WICED_ETHERTYPE_RARP )
        {
            _nx_rarp_packet_deferred_receive( ip_ptr[(int)interface], packet_ptr );
        }
        else
        {
            /* Unknown ethertype - just release the packet */
            nx_packet_release(packet_ptr);
        }
    }
}

/******************************************************************************
 Static functions
 *****************************************************************************/

static void nx_wiced_add_ethernet_header( NX_IP* ip_ptr_in, NX_PACKET* packet_ptr, ULONG destination_mac_msw, ULONG destination_mac_lsw, USHORT ethertype )
{
    ULONG* ethernet_header;

    /* Make space at the front of the packet buffer for the ethernet header */
    packet_ptr->nx_packet_prepend_ptr = packet_ptr->nx_packet_prepend_ptr - WICED_ETHERNET_SIZE;
    packet_ptr->nx_packet_length = packet_ptr->nx_packet_length + WICED_ETHERNET_SIZE;

    /* Ensure ethernet header writing starts with 32 bit alignment */
    ethernet_header = (ULONG *) ( packet_ptr->nx_packet_prepend_ptr - 2 );

    *ethernet_header = destination_mac_msw;
    *( ethernet_header + 1 ) = destination_mac_lsw;
    *( ethernet_header + 2 ) = ( ip_ptr_in->nx_ip_arp_physical_address_msw << 16 ) | ( ip_ptr_in->nx_ip_arp_physical_address_lsw >> 16 );
    *( ethernet_header + 3 ) = ( ip_ptr_in->nx_ip_arp_physical_address_lsw << 16 ) | ethertype;

    NX_CHANGE_ULONG_ENDIAN(*(ethernet_header));
    NX_CHANGE_ULONG_ENDIAN(*(ethernet_header+1));
    NX_CHANGE_ULONG_ENDIAN(*(ethernet_header+2));
    NX_CHANGE_ULONG_ENDIAN(*(ethernet_header+3));
}
