/*
 */
 
/*
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#ifndef _MQTT_CLIENT_DEMO_H_
#define _MQTT_CLIENT_DEMO_H_

     /* MQTT API header. */
#include "core_mqtt.h"

#include "transport_mbedtls.h"

/* Plaintext transport implementation. */
#include "plaintext_posix.h"

/*Include backoff algorithm header for retry logic.*/
#include "backoff_algorithm.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

/**
* @brief The length of the incoming publish records array used by the coreMQTT
* library to track QoS > 0 packet ACKS for incoming publishes.
*/
#define INCOMING_PUBLISH_RECORD_LEN         ( 10U )


/**
* @brief The length of the outgoing publish records array used by the coreMQTT
* library to track QoS > 0 packet ACKS for outgoing publishes.
*/
#define OUTGOING_PUBLISH_RECORD_LEN         ( 10U )

/**
 * @brief Maximum number of outgoing publishes maintained in the application
 * until an ack is received from the broker.
 */
#define MAX_OUTGOING_PUBLISHES                   ( 5U )

/**
 * @brief Size of the network buffer for MQTT packets.
 */
#define NETWORK_BUFFER_SIZE    ( 1024U )


typedef enum {
    MQTT_OVER_TCP,
	MQTT_OVER_SSL,	
}MQTT_TRANSPORT_TYPE_E;

typedef enum {
    MQTT_INIT,
	MQTT_CONNECTED,
	MQTT_DISCONNECT,
	MQTT_FORCE_DISCONNECT,
}MQTT_STATE_E;


/**
 * @brief Structure to keep the MQTT publish packets until an ack is received
 * for QoS2 publishes.
 */
typedef struct PublishPackets
{
    /**
     * @brief Packet identifier of the publish packet.
     */
    uint16_t packetId;

    /**
     * @brief Publish info of the publish packet.
     */
    MQTTPublishInfo_t pubInfo;
} PublishPackets_t;


typedef struct MQTTClientSession
{
    uint32_t sessionIndex;
    MQTTContext_t mqttContext;
    NetworkContext_t networkContext;
    PlaintextParams_t plaintextParams;
    TlsTransportParams_t tlsContext;
    NetworkCredentials_t tlsCredentials;
    MQTT_TRANSPORT_TYPE_E mqttTransportScheme;
    ServerInfo_t serverInfo; 
    MQTTConnectInfo_t connectInfo;
    MQTTPublishInfo_t lwtInfo;
    
    /**
     * @brief Array to track the incoming publish records for incoming publishes
     * with QoS > 0.
     *
     * This is passed into #MQTT_InitStatefulQoS to allow for QoS > 0.
     *
     */
    MQTTPubAckInfo_t pIncomingPublishRecords[ INCOMING_PUBLISH_RECORD_LEN ];

    /**
     * @brief Array to track the outgoing publish records for outgoing publishes
     * with QoS > 0.
     *
     * This is passed into #MQTT_InitStatefulQoS to allow for QoS > 0.
     *
     */
    MQTTPubAckInfo_t pOutgoingPublishRecords[ OUTGOING_PUBLISH_RECORD_LEN ];
    uint8_t buffer[NETWORK_BUFFER_SIZE];
    MQTT_STATE_E mqttState;
    uint32_t mqttPingRespCount;


    /**
     * @brief Packet Identifier updated when an ACK packet is received.
     *
     * It is used to match an expected ACK for a transmitted packet.
     */
    uint16_t ackPacketIdentifier;
    
    /**
     * @brief Packet Identifier generated when Subscribe request was sent to the broker;
     * it is used to match received Subscribe ACK to the transmitted subscribe.
     */
    uint16_t subscribePacketIdentifier;
    
    /**
     * @brief Packet Identifier generated when Unsubscribe request was sent to the broker;
     * it is used to match received Unsubscribe ACK to the transmitted unsubscribe
     * request.
     */
    uint16_t unsubscribePacketIdentifier;

    /**
     * @brief Packet Identifier generated when Publish message was sent to the broker;
     * it is used to match received Publish ACK to the transmitted Qos > 0 publish message.
     */
    uint16_t publishPacketIdentifier;

    /**
     * @brief Status of latest Subscribe ACK;
     * it is updated every time the callback function processes a Subscribe ACK
     * and accounts for subscription to a single topic.
     */
    MQTTSubAckStatus_t subAckStatus;

    
    /**
     * @brief Array to keep the outgoing publish messages.
     * These stored outgoing publish messages are kept until a successful ack
     * is received.
     */
    PublishPackets_t outgoingPublishPackets[ MAX_OUTGOING_PUBLISHES ];

} MQTTClientSession_t;


typedef enum {
    MQTT_CMD_NONE,
	MQTT_CMD_SUB,
    MQTT_CMD_UNSUB,
	MQTT_CMD_PUB,
	MQTT_CMD_DISC,	
}MQTT_CMD_TYPE_E;

typedef struct MQTTClientCMD
{
    MQTT_CMD_TYPE_E cmd_type;
    union {
        MQTTPublishInfo_t publish;
        MQTTSubscribeInfo_t subscribe;
        MQTTSubscribeInfo_t unsubscribe;
    }mqtt_cmd;
} MQTTClientCMD_t;

qapi_Status_t mqttc_demo(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

#endif
