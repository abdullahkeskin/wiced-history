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
#include "wiced.h"
#include "stdint.h"
#include "stddef.h"
#include "wwd_network_interface.h"
#include "wwd_buffer_interface.h"
#include "tlv.h"
#include "wiced_security.h"
#include "wiced_internal_api.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define GET_BIT(x, bit)     (x[bit / 32] & (1 << (bit&0x1F)))
#define SET_BIT(x, bit)     (x[bit / 32] |= (1 << (bit&0x1F)))

/******************************************************
 *                    Constants
 ******************************************************/

#define ETHERNET_ADDRESS_LENGTH    (6)

#define PACKET_SIZE_INDEX_OFFSET    (80)
#define MAX_NUMBER_OF_COOEE_BYTES   (512)

#define COOEE_CHANNEL_DWELL_TIME    (100 * MILLISECONDS)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)
typedef struct
{
    uint8_t type;
    uint8_t flags;
    uint16_t duration;
    uint8_t address1[ETHERNET_ADDRESS_LENGTH];
    uint8_t address2[ETHERNET_ADDRESS_LENGTH];
    uint8_t address3[ETHERNET_ADDRESS_LENGTH];
    uint16_t ether_type;
} ieee80211_header_t;

typedef struct
{
    uint8_t  header1;
    uint8_t  header2;
    uint8_t  nonce[8];
    uint8_t  data[1];
} wiced_cooee_header_t;

#pragma pack()

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef WICED_COOEE_ENABLE_SCANNING
static void cooee_scan_callback( wiced_scan_result_t** result_ptr, void* user_data );
#endif

static void process_packet(wiced_buffer_t buffer);
extern int rijndaelKeySetupEnc(uint32_t rk[], const uint8_t cipherKey[], int keyBits);

/******************************************************
 *               Variables Definitions
 ******************************************************/

static wiced_cooee_workspace_t* workspace;
static wiced_cooee_header_t* cooee_header;


/*
 * NOTE: Some of the following const objects are safely typecast to non-const variants
 * as wiced_wifi_add_packet_filter() only copies the contents. Generally this should
 * be avoided and is a bad thing to do.
 */
static const uint8_t cooee_upper_address_mask[]  = {0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t cooee_upper_address_match[] = {0x01, 0x00, 0x5e, 0x7e};

static const uint8_t cooee_beacon_address_mask[]  = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t cooee_beacon_address_match[] = {0x01, 0x00, 0x5e, 0x76, 0x00, 0x00};

static const wiced_packet_filter_pattern_t a1_pattern =
{
    .offset       = 4,
    .mask_size    = 4,
    .mask         = (uint8_t*)cooee_upper_address_mask,
    .pattern      = (uint8_t*)cooee_upper_address_match,
};

static const wiced_packet_filter_pattern_t a3_pattern =
{
    .offset       = 16,
    .mask_size    = 4,
    .mask         = (uint8_t*)cooee_upper_address_mask,
    .pattern      = (uint8_t*)cooee_upper_address_match,
};

static const wiced_packet_filter_pattern_t beacon_a1_pattern =
{
    .offset       = 4,
    .mask_size    = 4,
    .mask         = (uint8_t*)cooee_beacon_address_mask,
    .pattern      = (uint8_t*)cooee_beacon_address_match,
};

static const wiced_packet_filter_pattern_t beacon_a3_pattern =
{
    .offset       = 16,
    .mask_size    = 4,
    .mask         = (uint8_t*)cooee_beacon_address_mask,
    .pattern      = (uint8_t*)cooee_beacon_address_match,
};


static const wiced_packet_filter_settings_t a1_settings =
{
    .rule = WICED_PACKET_FILTER_RULE_POSITIVE_MATCHING,
    .pattern_count = 1,
    .pattern_list  = (wiced_packet_filter_pattern_t*)&a1_pattern,
};

static const wiced_packet_filter_settings_t a3_settings =
{
    .rule = WICED_PACKET_FILTER_RULE_POSITIVE_MATCHING,
    .pattern_count = 1,
    .pattern_list  = (wiced_packet_filter_pattern_t*)&a3_pattern,
};

static const wiced_packet_filter_settings_t beacon_a1_settings =
{
    .rule = WICED_PACKET_FILTER_RULE_POSITIVE_MATCHING,
    .pattern_count = 1,
    .pattern_list  = (wiced_packet_filter_pattern_t*)&beacon_a1_pattern,
};

static const wiced_packet_filter_settings_t beacon_a3_settings =
{
    .rule = WICED_PACKET_FILTER_RULE_POSITIVE_MATCHING,
    .pattern_count = 1,
    .pattern_list  = (wiced_packet_filter_pattern_t*)&beacon_a3_pattern,
};

/******************************************************
 *               Function Definitions
 ******************************************************/

#ifdef WICED_COOEE_ENABLE_SCANNING
static void cooee_scan_callback( wiced_scan_result_t** result_ptr, void* user_data )
{
    if (result_ptr == NULL)
    {
        scan_complete = WICED_TRUE;
    }
}
#endif

wiced_result_t wiced_wifi_cooee( wiced_cooee_workspace_t* cooee_workspace )
{
    wiced_scan_result_t ap;
    uint8_t             cooee_nonce[13];
    tlv8_data_t*        security_key_tlv;
    uint32_t            content_length;
    uint32_t            aes_key_rounds[60]; /* 4 * (AES_MAXROUNDS + 1) */
    tlv8_data_t*        tlv;

    workspace = cooee_workspace;
    cooee_header = (wiced_cooee_header_t*)workspace->received_cooee_data;

    wiced_wifi_add_packet_filter( 1, &a1_settings );
    wiced_wifi_add_packet_filter( 2, &a3_settings );
    wiced_wifi_add_packet_filter( 3, &beacon_a1_settings );
    wiced_wifi_add_packet_filter( 4, &beacon_a3_settings );

try_cooee_again:
    memset(&workspace->sniff_complete, 0, sizeof(workspace->sniff_complete));
    wiced_rtos_init_semaphore( &workspace->sniff_complete );
    workspace->received_byte_count  = 0;
    memset(workspace->received_segment_bitmap, 0, sizeof(workspace->received_segment_bitmap));
    memset(workspace->initiator_mac.octet, 0, sizeof(wiced_mac_t));
    memset(workspace->ap_bssid.octet, 0, sizeof(wiced_mac_t));
    workspace->wiced_cooee_complete = WICED_FALSE;

    wiced_wifi_enable_packet_filter( 1 );
    wiced_wifi_enable_packet_filter( 2 );
    wiced_wifi_enable_packet_filter( 3 );
    wiced_wifi_enable_packet_filter( 4 );

    wiced_wifi_enable_monitor_mode( );

    /* Scan through the channel list until we find something */
    wiced_mac_t bogus_scan_mac = {.octet={0,0,0,0,0,0}};
    wiced_bool_t initiator_details_printed = WICED_FALSE;
    uint16_t previous_received_byte_count = 0;

    while (wiced_rtos_get_semaphore( &workspace->sniff_complete, 1 * SECONDS ) == WICED_TIMEOUT)
    {
        if ( ( memcmp( workspace->initiator_mac.octet, bogus_scan_mac.octet, sizeof(wiced_mac_t) ) != 0 ) && ( initiator_details_printed != WICED_TRUE ) )
        {
            WPRINT_WICED_INFO(("\r\nEasy Setup detected\r\nTransmitter : %2X:%2X:%2X:%2X:%2X:%2X\r\nAccess Point: %2X:%2X:%2X:%2X:%2X:%2X\r\n",
                workspace->initiator_mac.octet[0], workspace->initiator_mac.octet[1], workspace->initiator_mac.octet[2], workspace->initiator_mac.octet[3], workspace->initiator_mac.octet[4], workspace->initiator_mac.octet[5],
                workspace->ap_bssid.octet[0], workspace->ap_bssid.octet[1], workspace->ap_bssid.octet[2], workspace->ap_bssid.octet[3], workspace->ap_bssid.octet[4], workspace->ap_bssid.octet[5] ));

            initiator_details_printed = WICED_TRUE;
        }

        if (workspace->received_byte_count != previous_received_byte_count)
        {
            previous_received_byte_count = workspace->received_byte_count;

            WPRINT_WICED_INFO(("%u of %u bytes received\r\n", workspace->received_byte_count, cooee_header->header2 ));
        }
#ifdef WICED_COOEE_ENABLE_SCANNING
        if (scan_complete == WICED_TRUE)
        {
            if (wiced_wifi_scan(WICED_SCAN_TYPE_PASSIVE, WICED_BSS_TYPE_INFRASTRUCTURE, NULL, &bogus_scan_mac, NULL, NULL, cooee_scan_callback, NULL, NULL) == WICED_SUCCESS)
            {
                scan_complete = WICED_FALSE;
            }
        }
#endif
    }

    WPRINT_WICED_INFO( ("\r\nCooee payload received\r\n") );

    wiced_wifi_disable_monitor_mode( );

    /* Prepare the nonce which is 8 bytes from header + "wiced" */
    memcpy( cooee_nonce, cooee_header->nonce, 8 );
    memcpy( &cooee_nonce[8], "wiced", 5 );

    /* Extract from header how much data there is */
    content_length = ( ( cooee_header->header1 & 0x0F ) << 8 ) + cooee_header->header2 - 10;

    /* Decrypt the data */
    memset( aes_key_rounds, 0, sizeof( aes_key_rounds ) );
    rijndaelKeySetupEnc( aes_key_rounds, wiced_dct_get_security_section( )->cooee_key, 128 );
    if (aes_ccm_decrypt( aes_key_rounds, 16, cooee_nonce, 10, (uint8_t*) cooee_header, content_length, &workspace->received_cooee_data[10], &workspace->received_cooee_data[10] ) != 0)
    {
        WPRINT_WICED_INFO( ("Cooee payload decryption failed\r\n") );
        goto return_with_error;
    }

    /* Process the content */
    memset(&ap, 0, sizeof(ap));
    tlv = (tlv8_data_t*) cooee_header->data;

    /* Process the mandatory SSID */
    if ( tlv->type != WICED_COOEE_SSID )
    {
        goto return_with_error;
    }
    ap.SSID.len = tlv->length;
    memcpy( ap.SSID.val, tlv->data, tlv->length );

    /* Process the mandatory security key */
    tlv = (tlv8_data_t*) ( &tlv->data[tlv->length] );
    if ( tlv->type != WICED_COOEE_WPA_KEY && tlv->type != WICED_COOEE_WEP_KEY )
    {
        goto return_with_error;
    }
    security_key_tlv = tlv;

    /* Process the mandatory host address */
    workspace->user_processed_data = &tlv->data[tlv->length];
//    tlv = (tlv8_data_t*) ( &tlv->data[tlv->length] );
//    if ( tlv->type != WICED_COOEE_IP_ADDRESS )
//    {
//        goto return_with_error;
//    }
//    if ( tlv->length == 4 )
//    {
//        memcpy( &host_ip_address->ip.v4, tlv->data, 4 );
//        host_ip_address->ip.v4 = htonl(host_ip_address->ip.v4);
//        host_ip_address->version = WICED_IPV4;
//    }
//    else if ( tlv->length == 6 )
//    {
//        memcpy( host_ip_address->ip.v6, tlv->data, 16 );
//        host_ip_address->version = WICED_IPV6;
//    }
//    else
//    {
//        goto return_with_error;
//    }

    WPRINT_WICED_INFO( ("SSID: %s\r\n", ap.SSID.val) );
    WPRINT_WICED_INFO( ("PSK : %.64s\r\n", security_key_tlv->data) );

    wiced_wifi_disable_packet_filter( 1 );
    wiced_wifi_disable_packet_filter( 2 );
    wiced_wifi_disable_packet_filter( 3 );
    wiced_wifi_disable_packet_filter( 4 );

    /* Setup the AP details */
    ap.security = WICED_SECURITY_WPA2_MIXED_PSK;
    ap.channel = 1;
    memcpy(ap.BSSID.octet, workspace->ap_bssid.octet, sizeof(wiced_mac_t));
    ap.band = WICED_802_11_BAND_2_4GHZ;
    ap.bss_type = WICED_BSS_TYPE_INFRASTRUCTURE;

    /* Store AP credentials into DCT */
    WPRINT_WICED_INFO( ("Storing received credentials in DCT\r\n\r\n") );
    platform_dct_wifi_config_t wifi_config_dct_local;
    wiced_dct_read_wifi_config_section( &wifi_config_dct_local );
    memcpy(&wifi_config_dct_local.stored_ap_list[0].details, &ap, sizeof(wiced_scan_result_t));
    wifi_config_dct_local.stored_ap_list[0].security_key_length = security_key_tlv->length;
    memcpy(wifi_config_dct_local.stored_ap_list[0].security_key, security_key_tlv->data, security_key_tlv->length);
    wiced_dct_write_wifi_config_section( (const platform_dct_wifi_config_t*)&wifi_config_dct_local );

    return WICED_SUCCESS;

return_with_error:
    WPRINT_WICED_INFO(("Easy Setup failed\r\n"));
    wiced_wifi_disable_packet_filter( 1 );
    wiced_wifi_disable_packet_filter( 2 );
    wiced_wifi_disable_packet_filter( 3 );
    wiced_wifi_disable_packet_filter( 4 );
    wiced_rtos_deinit_semaphore( &workspace->sniff_complete );
    goto try_cooee_again;
}

void host_network_process_raw_packet( wiced_buffer_t buffer, wiced_interface_t interface )
{
    if (workspace->wiced_cooee_complete == WICED_FALSE )
    {
        process_packet(buffer);
    }

    host_buffer_release( buffer, WICED_NETWORK_RX );
}

static void process_packet(wiced_buffer_t buffer)
{
    ieee80211_header_t* header;
    uint8_t* data = NULL;
    uint8_t* initiator = NULL;
    uint8_t* bssid = NULL;
    uint16_t extra_offset = 0;

    header = (ieee80211_header_t*) host_buffer_get_current_piece_data_pointer( buffer );

    if ( memcmp( header->address3, cooee_beacon_address_match, 6 ) == 0 )
    {
        if ( workspace->size_of_zero_data_packet == 0 )
        {
            workspace->size_of_zero_data_packet = host_buffer_get_current_piece_size( buffer ) - 2;
        }
        return;
    }
    else if ( memcmp( header->address1, cooee_beacon_address_match, 6 ) == 0 )
    {
        if ( workspace->size_of_zero_data_packet == 0 )
        {
            workspace->size_of_zero_data_packet = host_buffer_get_current_piece_size( buffer );
        }
        return;
    }

    /* Check if we have NOT found the Cooee beacon and know the size of a zero data length packet */
    if ( workspace->size_of_zero_data_packet == 0 )
    {
        return;
    }

    /* Check for Cooee data packets */
    if ( memcmp( header->address3, cooee_upper_address_match, 4 ) == 0 )
    {
        data      = &header->address3[4];
        initiator = header->address2;
        bssid     = header->address1;
        extra_offset = 2;
    }
    else if ( memcmp( header->address1, cooee_upper_address_match, 4 ) == 0 )
    {
        data      = &header->address1[4];
        initiator = header->address3;
        bssid     = header->address2;
    }
    else
    {
        return;
    }

    /* Make sure the packet length is valid */
    uint16_t packet_length = host_buffer_get_current_piece_size( buffer );
    if ( packet_length < MAX_NUMBER_OF_COOEE_BYTES + PACKET_SIZE_INDEX_OFFSET )
    {
        /* Check if this is the first packet we've heard */
        if ( workspace->received_byte_count == 0 )
        {
            /* Store the MAC address of the AP and the sender and only listen to this sender from now on */
            memcpy( workspace->initiator_mac.octet, initiator, sizeof(wiced_mac_t) );
            memcpy( workspace->ap_bssid.octet, bssid, sizeof(wiced_mac_t) );
        }

//        if ( memcmp( initiator_mac.octet, initiator, sizeof(wiced_mac_t) ) != 0 )
        {
            uint16_t index = ( packet_length - workspace->size_of_zero_data_packet - extra_offset );
//            if ( !( ( cooee_header->header2 != 0 ) && ( index >= cooee_header->header2 ) ) )
            {
                if ( GET_BIT(workspace->received_segment_bitmap, index/2) == 0 )
                {
                    workspace->received_cooee_data[index] = data[0];
                    workspace->received_cooee_data[index + 1] = data[1];
                    SET_BIT( workspace->received_segment_bitmap, index/2 );
                    workspace->received_byte_count += 2;

                    /* Check if we have all the data we need */
                    if ( ( cooee_header->header2 != 0 ) && ( workspace->received_byte_count >= cooee_header->header2 ) )
                    {
                        int a;
                        for (a = 0; a < cooee_header->header2/2; ++a)
                        {
                            if (GET_BIT(workspace->received_segment_bitmap, a) == 0)
                            {
                                return;
                            }
                        }
                        workspace->wiced_cooee_complete = WICED_TRUE;
                        wiced_rtos_set_semaphore( &workspace->sniff_complete );
                    }
                }
            }
        }
    }
}
