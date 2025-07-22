/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * AWS IoT Device SDK for Embedded C 202211.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Demo for showing the use of MQTT APIs to establish an MQTT session,
 * subscribe to a topic, publish to a topic, receive incoming publishes,
 * unsubscribe from a topic and disconnect the MQTT session.
 *
 * The example is single threaded and uses statically allocated memory;
 * it uses QOS2 and therefore implements a retransmission mechanism
 * for Publish messages. Retransmission of publish messages are attempted
 * when a MQTT connection is established with a session that was already
 * present. All the outgoing publish messages waiting to receive PUBREC
 * are resend in this demo. In order to support retransmission all the outgoing
 * publishes are stored until a PUBREC is received.
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

#include "qurt_internal.h"
#include "timer.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

/**
 * @brief The length of the incoming publish records array used by the coreMQTT
 * library to track QoS > 0 packet ACKS for incoming publishes.
 */
#define INCOMING_PUBLISH_RECORD_LEN (10U)

/**
 * @brief The length of the outgoing publish records array used by the coreMQTT
 * library to track QoS > 0 packet ACKS for outgoing publishes.
 */
#define OUTGOING_PUBLISH_RECORD_LEN (10U)

/**
 * @brief Maximum number of outgoing publishes maintained in the application
 * until an ack is received from the broker.
 */
#define MAX_OUTGOING_PUBLISHES (5U)

/**
 * @brief Size of the network buffer for MQTT packets.
 */
#define NETWORK_BUFFER_SIZE (1500U)

#define MQTT_DEMO_SESSION_NUM (2U)

/**
 * @brief Invalid packet identifier for the MQTT packets. Zero is always an
 * invalid packet identifier as per MQTT 3.1.1 spec.
 */
#define MQTT_PACKET_ID_INVALID ((uint16_t)0U)

/**
 * @brief Timeout for MQTT_ProcessLoop function in milliseconds.
 */
#define MQTT_PROCESS_LOOP_TIMEOUT_MS (500U)

/**
 * @brief The maximum time interval in seconds which is allowed to elapse
 *  between two Control Packets.
 *
 *  It is the responsibility of the Client to ensure that the interval between
 *  Control Packets being sent does not exceed the this Keep Alive value. In the
 *  absence of sending any other Control Packets, the Client MUST send a
 *  PINGREQ Packet.
 */
#define MQTT_KEEP_ALIVE_INTERVAL_SECONDS (60U)

/**
 * @brief The maximum number of retries for connecting to server.
 */
#define CONNECTION_RETRY_MAX_ATTEMPTS (3U)

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying connection to server.
 */
#define CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS (20000U)

/**
 * @brief The base back-off delay (in milliseconds) to use for connection retry attempts.
 */
#define CONNECTION_RETRY_BACKOFF_BASE_MS (10000U)

/**
 * @brief Timeout for receiving CONNACK packet in milli seconds.
 */
#define CONNACK_RECV_TIMEOUT_MS (5000U)

#define CLIENT_IDENTIFIER        ("testclient")                              /**< @brief Client identifier. */
#define CLIENT_IDENTIFIER_LENGTH ((uint16_t)(sizeof(CLIENT_IDENTIFIER) - 1)) /**< @brief Length of client identifier. \
                                                                              */

#define MQTT_EXAMPLE_TOPIC "example/topic"

/**
 * @brief Length of client MQTT topic.
 */
#define MQTT_EXAMPLE_TOPIC_LENGTH ((uint16_t)(sizeof(MQTT_EXAMPLE_TOPIC) - 1))

/**
 * @brief The MQTT message published in this example.
 */
#define MQTT_EXAMPLE_MESSAGE "Hello World!"

/**
 * @brief The length of the MQTT message published in this example.
 */
#define MQTT_EXAMPLE_MESSAGE_LENGTH ((uint16_t)(sizeof(MQTT_EXAMPLE_MESSAGE) - 1))

/* Check that transport timeout for transport send and receive is defined. */
#define TRANSPORT_SEND_RECV_TIMEOUT_MS (10)

/* Ping response timeout max time, otherwise will reconnect*/
#define MQTT_PING_RESP_TIMEOUT_MAX_TIMES (3)

/* Ping response timeout max time, otherwise will reconnect*/
#define MQTT_SUB_TOPIC_PER_SESSION_MAX (10)

typedef enum {
    MQTT_OVER_TCP,
    MQTT_OVER_SSL,
} MQTT_TRANSPORT_TYPE_E;

typedef enum {
    MQTT_INIT,
    MQTT_CONNECTED,
    MQTT_DISCONNECT,
    MQTT_FORCE_DISCONNECT,
} MQTT_STATE_E;

/**
 * @brief Structure to keep the MQTT publish packets until an ack is received
 * for QoS2 publishes.
 */
typedef struct PublishPackets {
    /**
     * @brief Packet identifier of the publish packet.
     */
    uint16_t packetId;

    /**
     * @brief Publish info of the publish packet.
     */
    MQTTPublishInfo_t pubInfo;
} PublishPackets_t;

typedef struct MQTTClientSession {
    uint32_t sessionIndex;
    MQTTContext_t mqttContext;
    NetworkContext_t networkContext;
    PlaintextParams_t plaintextParams;
    TlsTransportParams_t tlsContext;
    NetworkCredentials_t tlsCredentials;
    MQTT_TRANSPORT_TYPE_E mqttTransportScheme;
    ServerInfo_t serverInfo;
    MQTTSubscribeInfo_t subscribeInfo[MQTT_SUB_TOPIC_PER_SESSION_MAX];
    MQTTConnectInfo_t connectInfo;
    MQTTPublishInfo_t lwtInfo;

    /**
     * @brief Array to track the incoming publish records for incoming publishes
     * with QoS > 0.
     *
     * This is passed into #MQTT_InitStatefulQoS to allow for QoS > 0.
     *
     */
    MQTTPubAckInfo_t pIncomingPublishRecords[INCOMING_PUBLISH_RECORD_LEN];

    /**
     * @brief Array to track the outgoing publish records for outgoing publishes
     * with QoS > 0.
     *
     * This is passed into #MQTT_InitStatefulQoS to allow for QoS > 0.
     *
     */
    MQTTPubAckInfo_t pOutgoingPublishRecords[OUTGOING_PUBLISH_RECORD_LEN];
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
    PublishPackets_t outgoingPublishPackets[MAX_OUTGOING_PUBLISHES];

} MQTTClientSession_t;

typedef enum {
    MQTT_CMD_NONE,
    MQTT_CMD_SUB,
    MQTT_CMD_UNSUB,
    MQTT_CMD_PUB,
    MQTT_CMD_DISC,
} MQTT_CMD_TYPE_E;

typedef struct MQTTClientCMD {
    MQTT_CMD_TYPE_E cmd_type;
    union {
        MQTTPublishInfo_t publish;
        MQTTSubscribeInfo_t subscribe;
        MQTTSubscribeInfo_t unsubscribe;
    } mqtt_cmd;
} MQTTClientCMD_t;

typedef struct MQTTTaskCtrl {
    qurt_signal_t mqtt_client_signal;
    nt_osal_timer_handle_t mqtt_keepalive_timer;
    bool mqtt_keepalive_created;
    bool mqtt_signal_created;
    uint32_t mqttkeepalive_time_default;
    uint32_t mqttkeepalive_time_bmps;
} MQTTTaskCtrl_t;

extern MQTTClientSession_t mqtt_client_sess[MQTT_DEMO_SESSION_NUM];

qapi_Status_t mqttc_demo(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t mqttc_init(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t mqttc_connect(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t mqttc_subscribe(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t mqttc_unsubscribe(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t mqttc_publish(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t mqttc_publishRaw_Cache(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t mqttc_publishRaw_Block(uint32_t len, char *block_buf);
qapi_Status_t mqttc_disconnect(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t mqttc_destroy(uint32_t sessionIndex);
qapi_Status_t mqttc_connect_info_query(void);
qapi_Status_t mqttc_sub_info_query(void);

void cleanupNetworkCredentials(MQTTClientSession_t *pMqttClientSess);

#endif
