/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "apollocore.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/**
 * Events generated by the Apollo command sender service.
 * The application callback is invoked with the event and
 * associated event information passed via the arg parameter.
 */

typedef enum
{
    APOLLO_CMD_SENDER_EVENT_COMMAND_STATUS,         /* Arg is number of peers that responded returned */
    APOLLO_CMD_SENDER_EVENT_DISCOVER_RESULTS,       /* Arg is pointer to a apollo_peers_t structure   */
} APOLLO_CMD_SENDER_EVENT_T;


/**
 * Apollo command sender service commands.
 */

typedef enum
{
    APOLLO_CMD_SENDER_COMMAND_NONE              = 0,
    APOLLO_CMD_SENDER_COMMAND_DISCOVER_PEERS,       /* arg is ignored                            */
    APOLLO_CMD_SENDER_COMMAND_SPEAKER_CONFIG,       /* arg is pointer to apollo_speaker_config_t */
    APOLLO_CMD_SENDER_COMMAND_VOLUME,               /* arg is volume                             */
    APOLLO_CMD_SENDER_COMMAND_REBOOT                /* arg is ignored                            */
} APOLLO_CMD_SENDER_COMMAND_T;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/**
 * Speaker configuration structure passed with
 * APOLLO_CMD_SENDER_COMMAND_SPEAKER_CONFIG command.
 */
typedef struct apollo_speaker_config_s
{
    char speaker_name[APOLLO_SPEAKER_NAME_LENGTH];
    APOLLO_CHANNEL_MAP_T speaker_channel;
} apollo_speaker_config_t;

/**
 * Apollo speaker structure.
 */
typedef struct apollo_speaker_s
{
    wiced_mac_t mac;
    uint32_t ipaddr;
    apollo_speaker_config_t config;
} apollo_speaker_t;

/**
 * Apollo peers stucture passed to application with
 * APOLLO_CMD_SENDER_EVENT_DISCOVER_RESULTS event.
 * Contains the speaker information for each speaker discovered on the network.
 */
typedef struct apollo_peers_s
{
    int num_speakers;
    apollo_speaker_t speakers[APOLLO_MAX_SPEAKERS];
} apollo_peers_t;

/**
 * Apollo command sender service callback.
 * This routine is used by the service to notify the application of service events.
 *
 * @param handle    Service instance handle.
 * @param userdata  Application pointer from apollo_cmd_sender_init().
 * @param event     Apollo command sender service event.
 * @param arg       Pointer to event specific data structure.
 *
 * @return status of the application callback processing.
 */
typedef wiced_result_t (*apollo_cmd_sender_callback_t)(void* handle, void* userdata, APOLLO_CMD_SENDER_EVENT_T event, void* arg);

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/** Initialize the Apollo command sender service.
 *
 * @param[in] interface   : Interface for the command socket.
 * @param[in] userdata    : Userdata pointer passed back in event callback.
 * @param[in] callback    : Callback handler for command events.
 *
 * @return Pointer to the command instance or NULL
 */
void* apollo_cmd_sender_init(wiced_interface_t interface, void* userdata, apollo_cmd_sender_callback_t callback);


/** Deinitialize the Apollo command sender service.
 *
 * @param[in] handle  : Handle to the command instance.
 *
 * @return    Status of the operation.
 */
wiced_result_t apollo_cmd_sender_deinit(void *handle);

/** Issue a command to the Apollo command sender service.
 *
 * @param[in] handle     : Handle to the command instance.
 * @param[in] command    : Command to process.
 * @param[in] broadcast  : Flag indicating that the command is a broadcast command.
 * @param[in] target_mac : Target MAC address if not broadcast command.
 * @param[in] arg        : Command specific argument.
 *
 * @return    Status of the operation.
 *            WICED_WOULD_BLOCK if a command operation is currently in progress.
 */
wiced_result_t apollo_cmd_sender_command(void *handle, APOLLO_CMD_SENDER_COMMAND_T command, wiced_bool_t broadcast, wiced_mac_t* target_mac, void* arg);

#ifdef __cplusplus
} /* extern "C" */
#endif