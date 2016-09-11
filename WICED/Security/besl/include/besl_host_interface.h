/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include <stdint.h>
#include "besl_structures.h"

/* Endian management functions */
uint32_t besl_host_hton32(uint32_t intlong);
uint16_t besl_host_hton16(uint16_t intshort);
uint32_t besl_host_hton32_ptr(uint8_t* in, uint8_t* out);
uint16_t besl_host_hton16_ptr(uint8_t* in, uint8_t* out);
uint32_t besl_host_ntoh32(uint8_t* intlong);
uint16_t besl_host_ntoh16(uint8_t* intshort);
uint16_t besl_host_htol16(uint16_t intshort);
uint16_t besl_host_ltoh16(uint16_t intshort);


extern void besl_host_get_mac_address(besl_mac_t* address, uint32_t interface );
extern void besl_host_set_mac_address(besl_mac_t* address, uint32_t interface );
extern void besl_host_random_bytes(uint8_t* buffer, uint16_t buffer_length);

/* Memory allocation functions */
extern void* besl_host_malloc( char* name, uint32_t size );
extern void  besl_host_free( void* p );
