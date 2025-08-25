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

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

/* Standard includes. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "qapi_status.h"
#include "qapi_console.h"
#include "qurt_internal.h"

#include "qcli_api.h"

/* MQTT API headers. */
#include "core_mqtt.h"
#include "core_mqtt_state.h"

/*Include backoff algorithm header for retry logic.*/
#include "backoff_algorithm.h"

/* Clock for timer. */
#include "timer.h"

/* file system */
#include <unistd.h>
#include "fcntl.h"

/* assertion */
#include <assert.h>

#include "mqtt_client_demo.h"

#include "qat.h"
#include "qat_api.h"
#include <sys/socket.h>

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

#define MQTT_CLIENT_PRINTF(...) printf(__VA_ARGS__)

//#define MQTT_CLIENT_DEBUG_PRINT_ENABLE
#ifdef MQTT_CLIENT_DEBUG_PRINT_ENABLE
#define MQTT_CLIENT_DEBUG_PRINTF(...) \
    do {                              \
        printf(__VA_ARGS__);          \
    } while (0);

#else
#define MQTT_CLIENT_DEBUG_PRINTF(...)
#endif

#define WRTMEM_STR_BUFFER_LENGTH 1500

#define MQTT_KEEP_ALIVE_INTERVAL_MSECONDS 60000

#define MQTT_PUB_RAW_START 0x1
#define MQTT_PUB_RAW_DONE  0x2
#define MQTT_PUB_KEEPALIVE 0x4
#define MQTT_SUB           0x8
#define MQTT_UNSUB         0x10
#define MQTT_DISCONN       0x20
#define MQTT_PKT_RECV      0x40
/*-------------------------------------------------------------------------
 * Variables
 *-----------------------------------------------------------------------*/

MQTTClientSession_t mqtt_client_sess[MQTT_DEMO_SESSION_NUM];
MQTTClientCMD_t mqtt_client_cmd[MQTT_DEMO_SESSION_NUM];
bool mqttThreadCreated = false;
bool mqttRxThreadCreated = false;

MQTTTaskCtrl_t mqtt_task_ctrl = {.mqtt_keepalive_timer = NULL,
                                 .mqtt_client_signal = 0,
                                 .mqtt_keepalive_created = false,
                                 .mqtt_signal_created = false,
                                 .mqttkeepalive_time_bmps = MQTT_KEEP_ALIVE_INTERVAL_MSECONDS};

#ifdef CONFIG_QAT_MQTT_DEMO
extern uint8_t isRecvHex;
#endif

static uint32_t sessionIndex_raw;
static uint32_t total_len_one_raw;
static uint32_t total_send_len_one_raw = 0;
/*-------------------------------------------------------------------------
 * Private Function Declarations
 *-----------------------------------------------------------------------*/

/**
 * @brief Function to clean up all the outgoing publishes maintained in the
 * array.
 */
static void cleanupOutgoingPublishes(PublishPackets_t *pOutgoingPublishPackets);

/**
 * @brief Function to resend the publishes if a session is re-established with
 * the broker or the publish QoS >0 failed. This function handles the resending of the QoS>0 and  publish packets,
 * which are maintained locally.
 *
 * @param[in] pOutgoingPublishPackets  Array to keep the outgoing publish messages pointer.
 * @param[in] pMqttContext MQTT context pointer.
 */
static int handlePublishResend(PublishPackets_t *pOutgoingPublishPackets, MQTTContext_t *pMqttContext);

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/* Function for obtaining a timestamp. */
uint32_t getTimeStampMs()
{
    return hres_timer_curr_time_ms();
}

/**
 * @brief The random number generator to use for exponential backoff with
 * jitter retry logic.
 *
 * @return The generated random number.
 */
static uint32_t generateRandomNumber()
{
    return (rand());
}

/**
 * @brief Connect to MQTT broker with reconnection retries.
 *
 * If connection fails, retry is attempted after a timeout.
 * Timeout value will exponentially increase until maximum
 * timeout value is reached or the number of attempts are exhausted.
 *
 * @param[in,out] pMqttClientSess The connection session context.
 *
 * @param[in] maxAttempts The maximum number of retry attempts. Set the value to
 * #BACKOFF_ALGORITHM_RETRY_FOREVER to retry for ever.
 *
 * @return EXIT_FAILURE on failure; EXIT_SUCCESS on successful connection.
 */
static int connectToServerWithBackoffRetries(MQTTClientSession_t *pMqttClientSess, uint32_t maxAttempts)
{
    int returnStatus = EXIT_SUCCESS;
    BackoffAlgorithmStatus_t backoffAlgStatus = BackoffAlgorithmSuccess;
    bool sessionPresent;
    MQTTStatus_t mqttStatus;
    BackoffAlgorithmContext_t reconnectParams;

    uint16_t nextRetryBackOff = 0U;
    NetworkContext_t *pNetworkContext = &pMqttClientSess->networkContext;
    MQTTContext_t *pMqttContext = &pMqttClientSess->mqttContext;

    /* Initialize reconnect attempts and interval */
    BackoffAlgorithm_InitializeParams(&reconnectParams, CONNECTION_RETRY_BACKOFF_BASE_MS,
                                      CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS, maxAttempts);

    /* Attempt to connect to MQTT broker. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase till maximum
     * attempts are reached.
     */
    do {
        if (pMqttClientSess->mqttTransportScheme == MQTT_OVER_TCP) {
            /* Establish a TCP connection with the MQTT broker. This example connects
             * to the MQTT broker as specified in pMqttClientSess->serverInfo. */
            MQTT_CLIENT_PRINTF("MQTT session:%d creating a TCP connection to %s:%d.\n", pMqttClientSess->sessionIndex,
                               pMqttClientSess->serverInfo.pHostName, pMqttClientSess->serverInfo.port);
            SocketStatus_t socketStatus = SOCKETS_SUCCESS;
            socketStatus = Plaintext_Connect(pNetworkContext, &pMqttClientSess->serverInfo,
                                             TRANSPORT_SEND_RECV_TIMEOUT_MS, TRANSPORT_SEND_RECV_TIMEOUT_MS);

            if (socketStatus != SOCKETS_SUCCESS) {
                MQTT_CLIENT_PRINTF("MQTT session:%d TCP connection failed, socketStatus = %x.\n",
                                   pMqttClientSess->sessionIndex, socketStatus);
                returnStatus = EXIT_FAILURE;
            } else {
                MQTT_CLIENT_PRINTF("MQTT session:%d TCP connection successfully established with broker.\n",
                                   pMqttClientSess->sessionIndex);
                returnStatus = EXIT_SUCCESS;
            }
        } else if (pMqttClientSess->mqttTransportScheme == MQTT_OVER_SSL) {
            /* Establish a TLS session with the MQTT broker. This example connects
             * to the MQTT broker as specified in pMqttClientSess->serverInfo. */
            MQTT_CLIENT_PRINTF("MQTT session:%d establishing a TLS session to %s:%d\n", pMqttClientSess->sessionIndex,
                               pMqttClientSess->serverInfo.pHostName, pMqttClientSess->serverInfo.port);

            TlsTransportStatus_t tlsStatus = TLS_TRANSPORT_SUCCESS;
            tlsStatus = TLS_FreeRTOS_Connect(pNetworkContext, pMqttClientSess->serverInfo.pHostName,
                                             pMqttClientSess->serverInfo.port, &(pMqttClientSess->tlsCredentials),
                                             TRANSPORT_SEND_RECV_TIMEOUT_MS);

            if (tlsStatus != TLS_TRANSPORT_SUCCESS) {
                MQTT_CLIENT_PRINTF("MQTT session:%d TLS connection failed tlsStatus = %d.\n",
                                   pMqttClientSess->sessionIndex, tlsStatus);
                returnStatus = EXIT_FAILURE;
            } else {
                MQTT_CLIENT_PRINTF("MQTT session:%d TLS connection successfully established with broker.\n",
                                   pMqttClientSess->sessionIndex);
                returnStatus = EXIT_SUCCESS;
            }
        }

        if (returnStatus == EXIT_SUCCESS) {
            /* Establish MQTT session on top of TCP/TLS connection. */
            MQTT_CLIENT_PRINTF("MQTT session:%d creating an MQTT connection to %s:%d.\n", pMqttClientSess->sessionIndex,
                               pMqttClientSess->serverInfo.pHostName, pMqttClientSess->serverInfo.port);

            /* Send MQTT CONNECT packet to broker, and waits for connection acknowledgment (CONNACK) packet.*/
            mqttStatus = MQTT_Connect(pMqttContext, &pMqttClientSess->connectInfo, &pMqttClientSess->lwtInfo,
                                      CONNACK_RECV_TIMEOUT_MS, &sessionPresent);

            if (mqttStatus != MQTTSuccess) {
                returnStatus = EXIT_FAILURE;
                MQTT_CLIENT_PRINTF("MQTT session:%d MQTT connection with MQTT broker failed with status %s.\n",
                                   pMqttClientSess->sessionIndex, MQTT_Status_strerror(mqttStatus));
            } else {
                MQTT_CLIENT_PRINTF("MQTT session:%d MQTT connection successfully established with broker.\n",
                                   pMqttClientSess->sessionIndex);

                if ((pMqttClientSess->connectInfo.cleanSession == false) && (sessionPresent == true)) {
                    MQTT_CLIENT_PRINTF("An MQTT session with broker is re-established. Resending unacked publishes.\n");
                    /* Handle all the resend of publish messages. */
                    handlePublishResend(pMqttClientSess->outgoingPublishPackets, pMqttContext);

                } else {
                    MQTT_CLIENT_PRINTF(
                        "A clean MQTT connection is established. Cleaning up all the stored outgoing publishes.\n");

                    /* Clean up the outgoing publishes waiting for ack as this new
                     * connection doesn't re-establish an existing session. */
                    cleanupOutgoingPublishes(pMqttClientSess->outgoingPublishPackets);
                }
            }

            if (returnStatus == EXIT_FAILURE) {
                if (pMqttClientSess->mqttTransportScheme == MQTT_OVER_TCP) {
                    /* Close the TCP connection.  */
                    (void)Plaintext_Disconnect(pNetworkContext);
                } else if (pMqttClientSess->mqttTransportScheme == MQTT_OVER_SSL) {
                    /* Close the TLS connection.  */
                    (void)TLS_FreeRTOS_Disconnect(pNetworkContext);
                }
            }
        }

        if (returnStatus == EXIT_FAILURE) {
            /* Generate a random number and get back-off value (in milliseconds) for the next connection retry. */
            backoffAlgStatus =
                BackoffAlgorithm_GetNextBackoff(&reconnectParams, generateRandomNumber(), &nextRetryBackOff);

            if (backoffAlgStatus == BackoffAlgorithmRetriesExhausted) {
                MQTT_CLIENT_DEBUG_PRINTF("MQTT session:%d connection to the broker failed, all attempts exhausted.\n",
                                         pMqttClientSess->sessionIndex);
                returnStatus = EXIT_FAILURE;
            } else if (backoffAlgStatus == BackoffAlgorithmSuccess) {
                MQTT_CLIENT_PRINTF(
                    "MQTT session:%d connection to the broker failed. Retrying connection "
                    "after %u ms backoff.\n",
                    pMqttClientSess->sessionIndex, (unsigned short)nextRetryBackOff);
                qurt_thread_sleep(nextRetryBackOff);
            }
        }
    } while ((returnStatus == EXIT_FAILURE) && (backoffAlgStatus == BackoffAlgorithmSuccess));

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int getNextFreeIndexForOutgoingPublishes(PublishPackets_t *pOutgoingPublishPackets, uint8_t *pIndex)
{
    int returnStatus = EXIT_FAILURE;
    uint8_t index = 0;

    assert(pOutgoingPublishPackets != NULL);
    assert(pIndex != NULL);

    for (index = 0; index < MAX_OUTGOING_PUBLISHES; index++) {
        /* A free index is marked by invalid packet id.
         * Check if the the index has a free slot. */
        if (pOutgoingPublishPackets[index].packetId == MQTT_PACKET_ID_INVALID) {
            returnStatus = EXIT_SUCCESS;

            /* Copy the available index into the output param. */
            *pIndex = index;

            break;
        }
    }

    /* Copy the available index into the output param. */
    *pIndex = index;

    return returnStatus;
}
/*-----------------------------------------------------------*/

static void cleanupOutgoingPublishAt(PublishPackets_t *pOutgoingPublishPackets, uint8_t index)
{
    assert(pOutgoingPublishPackets != NULL);
    assert(index < MAX_OUTGOING_PUBLISHES);

    if (pOutgoingPublishPackets[index].pubInfo.pPayload) {
        free((char *)pOutgoingPublishPackets[index].pubInfo.pPayload);
        pOutgoingPublishPackets[index].pubInfo.pPayload = NULL;
    }

    if (pOutgoingPublishPackets[index].pubInfo.pTopicName) {
        free((char *)pOutgoingPublishPackets[index].pubInfo.pTopicName);
        pOutgoingPublishPackets[index].pubInfo.pTopicName = NULL;
    }

    /* Clear the outgoing publish packet. */
    (void)memset(&(pOutgoingPublishPackets[index]), 0x00, sizeof(pOutgoingPublishPackets[index]));
}

/*-----------------------------------------------------------*/

static void cleanupOutgoingPublishes(PublishPackets_t *pOutgoingPublishPackets)
{
    assert(pOutgoingPublishPackets != NULL);

    /* Clean up all the outgoing publish packets. */
    (void)memset(pOutgoingPublishPackets, 0x00, sizeof(pOutgoingPublishPackets));
}

/*-----------------------------------------------------------*/

static void cleanupOutgoingPublishWithPacketID(PublishPackets_t *pOutgoingPublishPackets, uint16_t packetId)
{
    uint8_t index = 0;

    assert(pOutgoingPublishPackets != NULL);
    assert(packetId != MQTT_PACKET_ID_INVALID);

    /* Clean up all the saved outgoing publishes. */
    for (; index < MAX_OUTGOING_PUBLISHES; index++) {
        if (pOutgoingPublishPackets[index].packetId == packetId) {
            cleanupOutgoingPublishAt(pOutgoingPublishPackets, index);
            MQTT_CLIENT_DEBUG_PRINTF("Cleaned up outgoing publish packet with packet id %u.\n", packetId);
            break;
        }
    }
}

/*-----------------------------------------------------------*/

static int handlePublishResend(PublishPackets_t *pOutgoingPublishPackets, MQTTContext_t *pMqttContext)
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    uint8_t index = 0U;
    MQTTStateCursor_t cursor = MQTT_STATE_CURSOR_INITIALIZER;
    uint16_t packetIdToResend = MQTT_PACKET_ID_INVALID;
    bool foundPacketId = false;

    assert(pMqttContext != NULL);
    assert(pOutgoingPublishPackets != NULL);

    /* MQTT_PublishToResend() provides a packet ID of the next PUBLISH packet
     * that should be resent. In accordance with the MQTT v3.1.1 spec,
     * MQTT_PublishToResend() preserves the ordering of when the original
     * PUBLISH packets were sent. The outgoingPublishPackets array is searched
     * through for the associated packet ID. If the application requires
     * increased efficiency in the look up of the packet ID, then a hashmap of
     * packetId key and PublishPacket_t values may be used instead. */
    packetIdToResend = MQTT_PublishToResend(pMqttContext, &cursor);

    while (packetIdToResend != MQTT_PACKET_ID_INVALID) {
        foundPacketId = false;

        for (index = 0U; index < MAX_OUTGOING_PUBLISHES; index++) {
            if (pOutgoingPublishPackets[index].packetId == packetIdToResend) {
                foundPacketId = true;
                pOutgoingPublishPackets[index].pubInfo.dup = true;

                MQTT_CLIENT_PRINTF("Sending duplicate PUBLISH with packet id %u.\n",
                                   pOutgoingPublishPackets[index].packetId);
                mqttStatus = MQTT_Publish(pMqttContext, &pOutgoingPublishPackets[index].pubInfo,
                                          pOutgoingPublishPackets[index].packetId);

                if (mqttStatus != MQTTSuccess) {
                    MQTT_CLIENT_PRINTF("Sending duplicate PUBLISH for packet id %u failed with status %s.\n",
                                       pOutgoingPublishPackets[index].packetId, MQTT_Status_strerror(mqttStatus));
                    returnStatus = EXIT_FAILURE;
                    break;
                } else {
                    MQTT_CLIENT_PRINTF("Sent duplicate PUBLISH successfully for packet id %u.\n",
                                       pOutgoingPublishPackets[index].packetId);
                }
            }
        }

        if (foundPacketId == false) {
            MQTT_CLIENT_PRINTF("Packet id %u requires resend, but was not found in outgoingPublishPackets.\n",
                               packetIdToResend);
            returnStatus = EXIT_FAILURE;
            break;
        } else {
            /* Get the next packetID to be resent. */
            packetIdToResend = MQTT_PublishToResend(pMqttContext, &cursor);
        }
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static void handleIncomingPublish(MQTTPublishInfo_t *pPublishInfo, uint16_t packetIdentifier, uint32_t sessionIndex)
{
    assert(pPublishInfo != NULL);
    (void)packetIdentifier;
    int i = 0;

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
    uint32_t offset = 0;
    char *ptr = buffer;
#endif

    /* Process incoming Publish. */
#ifdef CONFIG_QAT_MQTT_DEMO
    char *ptopic = malloc(pPublishInfo->topicNameLength + 1);
    if (ptopic) {
        for (i = 0; i < pPublishInfo->topicNameLength; i++) {
            ptopic[i] = pPublishInfo->pTopicName[i];
        }
        if (isRecvHex) {
            ptopic[pPublishInfo->topicNameLength] = '\0';
            offset = snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_SUBRECVHEX:%d,\"%s\",%d,", sessionIndex,
                              ptopic, pPublishInfo->payloadLength);
        } else {
            ptopic[pPublishInfo->topicNameLength] = '\0';
            offset = snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_SUBRECV:%d,\"%s\",%d,", sessionIndex, ptopic,
                              pPublishInfo->payloadLength);
        }
        // QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
        free(ptopic);
    }

#else
    MQTT_CLIENT_PRINTF("Incoming Publish Topic Name : ");
#endif

#ifdef CONFIG_QAT_MQTT_DEMO
    // snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "%s,", pPublishInfo->pTopicName);
    // QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
#else
    for (i = 0; i < pPublishInfo->topicNameLength; i++) {
        MQTT_CLIENT_PRINTF("%c", pPublishInfo->pTopicName[i]);
    }
#endif

#ifdef CONFIG_QAT_MQTT_DEMO
    // snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "Qos : %d.\n", pPublishInfo->qos);
    // QAT_Response_Str(QAT_RC_QUIET, buffer);

    // snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "Incoming Publish Message : ");
    // QAT_Response_Str(QAT_RC_QUIET, buffer);

#else
    MQTT_CLIENT_PRINTF(" Qos : %d.\n", pPublishInfo->qos);

    MQTT_CLIENT_PRINTF("Incoming Publish message Packet Id is %u.\n", packetIdentifier);
    MQTT_CLIENT_PRINTF("Incoming Publish Message : ");
#endif

#ifdef CONFIG_QAT_MQTT_DEMO
    // memset(buffer,0,WRTMEM_STR_BUFFER_LENGTH);
    ptr += offset;
#endif

    for (size_t i = 0; i < pPublishInfo->payloadLength; i++) {
#ifdef CONFIG_QAT_MQTT_DEMO
        // snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "%c", ((const char *) pPublishInfo->pPayload)[i]);
        if (isRecvHex) {
            ptr[i] = ((const char *)(pPublishInfo->pPayload))[i];
        } else {
            ptr[i] = (char)((const char *)pPublishInfo->pPayload)[i];
        }

#else
        MQTT_CLIENT_PRINTF("%c", ((const char *)pPublishInfo->pPayload)[i]);
#endif
    }

#ifdef CONFIG_QAT_MQTT_DEMO
    if (!isRecvHex) {
        ptr[pPublishInfo->payloadLength] = '\0';
    }
#endif

#ifdef CONFIG_QAT_MQTT_DEMO
    if (isRecvHex) {
        QAT_Response_Buffer(QAT_RC_QUIET, buffer, offset + pPublishInfo->payloadLength);
    } else {
        QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
    }

#endif

#ifdef CONFIG_QAT_MQTT_DEMO
    QAT_Response_Str(QAT_RC_QUIET, NULL);
#else
    MQTT_CLIENT_PRINTF("\n");
#endif
}

/*-----------------------------------------------------------*/

static void updateSubAckStatus(MQTTClientSession_t *pMqttClientSess, MQTTPacketInfo_t *pPacketInfo)
{
    uint8_t *pPayload = NULL;
    size_t pSize = 0;
    assert(pMqttClientSess != NULL);
    MQTTStatus_t mqttStatus = MQTT_GetSubAckStatusCodes(pPacketInfo, &pPayload, &pSize);

    /* MQTT_GetSubAckStatusCodes always returns success if called with packet info
     * from the event callback and non-NULL parameters. */
    assert(mqttStatus == MQTTSuccess);

    /* Suppress unused variable warning when asserts are disabled in build. */
    (void)mqttStatus;

    /* Demo only subscribes to one topic, so only one status code is returned. */
    pMqttClientSess->subAckStatus = (MQTTSubAckStatus_t)pPayload[0];
}

/*-----------------------------------------------------------*/

static void eventCallback(MQTTContext_t *pMqttContext, MQTTPacketInfo_t *pPacketInfo,
                          MQTTDeserializedInfo_t *pDeserializedInfo)
{
    uint16_t packetIdentifier;

    assert(pMqttContext != NULL);
    assert(pPacketInfo != NULL);
    assert(pDeserializedInfo != NULL);

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
#endif

    /* Suppress unused parameter warning when asserts are disabled in build. */
    (void)pMqttContext;

    MQTTClientSession_t *pMqttClientSess =
        (MQTTClientSession_t *)((char *)pMqttContext - ((size_t) & ((MQTTClientSession_t *)0)->mqttContext));

    assert(&pMqttClientSess->mqttContext == pMqttContext);

    packetIdentifier = pDeserializedInfo->packetIdentifier;

#ifdef CONFIG_QAT_MQTT_DEMO
    // snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "MQTT session:%d received message.\n",
    //                     pMqttClientSess->sessionIndex);
    // QAT_Response_Str(QAT_RC_QUIET, buffer);
#else
    MQTT_CLIENT_PRINTF("MQTT session:%d received message.\n", pMqttClientSess->sessionIndex);
#endif

    /* Handle incoming publish. The lower 4 bits of the publish packet
     * type is used for the dup, QoS, and retain flags. Hence masking
     * out the lower bits to check if the packet is publish. */
    if ((pPacketInfo->type & 0xF0U) == MQTT_PACKET_TYPE_PUBLISH) {
        assert(pDeserializedInfo->pPublishInfo != NULL);
        /* Handle incoming publish. */
        handleIncomingPublish(pDeserializedInfo->pPublishInfo, packetIdentifier, pMqttClientSess->sessionIndex);
    } else {
        /* Handle other packets. */
        switch (pPacketInfo->type) {
            case MQTT_PACKET_TYPE_SUBACK:
                MQTT_CLIENT_PRINTF("SUBACK received for packet id %u.\n", packetIdentifier);

                /* A SUBACK from the broker, containing the server response to our subscription request, has been
                 * received. It contains the status code indicating server approval/rejection for the subscription to
                 * the single topic requested. The SUBACK will be parsed to obtain the status code, and this status code
                 * will be stored in global variable globalSubAckStatus. */
                updateSubAckStatus(pMqttClientSess, pPacketInfo);

                /* Make sure ACK packet identifier matches with Request packet identifier. */
                if (pMqttClientSess->subscribePacketIdentifier == packetIdentifier) {
                    /* Update the global ACK packet identifier. */
                    pMqttClientSess->ackPacketIdentifier = packetIdentifier;
                    pMqttClientSess->ackPacketIdentifier = packetIdentifier;
                }

                break;

            case MQTT_PACKET_TYPE_UNSUBACK:
                MQTT_CLIENT_PRINTF("UNSUBACK received for packet id %u.\n", packetIdentifier);

                /* Make sure ACK packet identifier matches with Request packet identifier. */
                if (pMqttClientSess->unsubscribePacketIdentifier == packetIdentifier) {
                    /* Update the global ACK packet identifier. */
                    pMqttClientSess->ackPacketIdentifier = packetIdentifier;
                }

                break;

            case MQTT_PACKET_TYPE_PINGRESP:

                /* Nothing to be done from application as library handles
                 * PINGRESP. */
                MQTT_CLIENT_PRINTF(
                    "PINGRESP should not be handled by the application "
                    "callback when using MQTT_ProcessLoop.\n");
                break;

            case MQTT_PACKET_TYPE_PUBACK:
                MQTT_CLIENT_PRINTF("PUBACK received for packet id %u.\n", packetIdentifier);

                /* Make sure ACK packet identifier matches with Request packet identifier. */
                if (pMqttClientSess->publishPacketIdentifier == packetIdentifier) {
                    /* Update the global ACK packet identifier. */
                    pMqttClientSess->ackPacketIdentifier = packetIdentifier;
                }

                /* Cleanup publish packet when a PUBACK is received. */
                cleanupOutgoingPublishWithPacketID(pMqttClientSess->outgoingPublishPackets, packetIdentifier);
                break;

            case MQTT_PACKET_TYPE_PUBREC:
                MQTT_CLIENT_PRINTF("PUBREC received for packet id %u.\n", packetIdentifier);
                /* Cleanup publish packet when a PUBREC is received. */
                cleanupOutgoingPublishWithPacketID(pMqttClientSess->outgoingPublishPackets, packetIdentifier);
                break;

            case MQTT_PACKET_TYPE_PUBREL:

                /* Nothing to be done from application as library handles
                 * PUBREL. */
                MQTT_CLIENT_PRINTF("PUBREL received for packet id %u.\n", packetIdentifier);
                break;

            case MQTT_PACKET_TYPE_PUBCOMP:

                /* Nothing to be done from application as library handles
                 * PUBCOMP. */
                MQTT_CLIENT_PRINTF("PUBCOMP received for packet id %u.\n", packetIdentifier);

                if (pMqttClientSess->publishPacketIdentifier == packetIdentifier) {
                    /* Update the global ACK packet identifier. */
                    pMqttClientSess->ackPacketIdentifier = packetIdentifier;
                }

                break;

            /* Any other packet type is invalid. */
            default:
                MQTT_CLIENT_PRINTF("Unknown packet type received:(%02x).\n", pPacketInfo->type);
        }
    }
}

static int publishToTopic(MQTTContext_t *pMqttContext, MQTTClientCMD_t *pMqttCommand,
                          MQTTClientSession_t *pMqttClientSess)
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    uint16_t packetId;
#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
#endif
    assert(pMqttContext != NULL);

    packetId = MQTT_GetPacketId(pMqttContext);

    /* Send PUBLISH packet. Packet Id is not used for a QoS0 publish.
     * Hence 0 is passed as packet id. */
    mqttStatus = MQTT_Publish(pMqttContext, &pMqttCommand->mqtt_cmd.publish, packetId);

    if (mqttStatus != MQTTSuccess) {
        MQTT_CLIENT_PRINTF("Failed to send PUBLISH packet to broker with error = %s.\n",
                           MQTT_Status_strerror(mqttStatus));
    } else {
        // MQTT_CLIENT_PRINTF("PUBLISH sent for topic %s to broker.\n",
        //                   pMqttCommand->mqtt_cmd.publish.pTopicName);
    }

    if (pMqttCommand->mqtt_cmd.publish.pPayload) {
        free((char *)pMqttCommand->mqtt_cmd.publish.pPayload);
        pMqttCommand->mqtt_cmd.publish.pPayload = NULL;
    }
    pMqttCommand->cmd_type = MQTT_CMD_NONE;

    total_send_len_one_raw += pMqttCommand->mqtt_cmd.publish.payloadLength;
    if (total_send_len_one_raw >= total_len_one_raw) {
#ifdef CONFIG_QAT_MQTT_DEMO
        if (mqttStatus == MQTTSuccess) {
            snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_PUBSUC:%d,%d,%d,\"%s\",%d",
                     pMqttClientSess->sessionIndex, total_send_len_one_raw, pMqttClientSess->mqttTransportScheme,
                     pMqttClientSess->serverInfo.pHostName, pMqttClientSess->serverInfo.port);
            QAT_Response_Str(QAT_RC_QUIET, buffer);
        } else {
            snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_PUBFAIL:%d,%d,\"%s\",%d",
                     pMqttClientSess->sessionIndex, pMqttClientSess->mqttTransportScheme,
                     pMqttClientSess->serverInfo.pHostName, pMqttClientSess->serverInfo.port);
            QAT_Response_Str(QAT_RC_QUIET, buffer);
        }
#endif
        if (pMqttCommand->mqtt_cmd.publish.pTopicName) {
            free(pMqttCommand->mqtt_cmd.publish.pTopicName);
        }
        pMqttCommand->mqtt_cmd.publish.pTopicName = NULL;
        memset(pMqttCommand, 0, sizeof(MQTTClientCMD_t));

        total_len_one_raw = 0;
        total_send_len_one_raw = 0;
        QAT_Transfer_Mode_set(QAT_Transfer_Mode_AT_COMMAND_E, NULL);
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int waitForPacketAck(MQTTClientSession_t *pMqttClientSess, uint16_t usPacketIdentifier, uint32_t ulTimeout)
{
    uint32_t ulMqttProcessLoopEntryTime;
    uint32_t ulMqttProcessLoopTimeoutTime;
    uint32_t ulCurrentTime;

    MQTTStatus_t eMqttStatus = MQTTSuccess;
    int returnStatus = EXIT_FAILURE;
    MQTTContext_t *pMqttContext = &pMqttClientSess->mqttContext;
    /* Reset the ACK packet identifier being received. */
    pMqttClientSess->ackPacketIdentifier = 0U;

    ulCurrentTime = pMqttContext->getTime();
    ulMqttProcessLoopEntryTime = ulCurrentTime;
    ulMqttProcessLoopTimeoutTime = ulCurrentTime + ulTimeout;

    /* Call MQTT_ProcessLoop multiple times until the expected packet ACK
     * is received, a timeout happens, or MQTT_ProcessLoop fails. */
    while ((pMqttClientSess->ackPacketIdentifier != usPacketIdentifier) &&
           (ulCurrentTime < ulMqttProcessLoopTimeoutTime) &&
           (eMqttStatus == MQTTSuccess || eMqttStatus == MQTTNeedMoreBytes)) {
        /* Event callback will set #globalAckPacketIdentifier when receiving
         * appropriate packet. */
        eMqttStatus = MQTT_ProcessLoop(pMqttContext);
        ulCurrentTime = pMqttContext->getTime();
    }

    if (((eMqttStatus != MQTTSuccess) && (eMqttStatus != MQTTNeedMoreBytes)) ||
        (pMqttClientSess->ackPacketIdentifier != usPacketIdentifier)) {
        MQTT_CLIENT_PRINTF(
            "MQTT_ProcessLoop failed to receive ACK packet: Expected ACK Packet ID=%02X, LoopDuration=%u, Status=%s\n",
            usPacketIdentifier, (ulCurrentTime - ulMqttProcessLoopEntryTime), MQTT_Status_strerror(eMqttStatus));
    } else {
        returnStatus = EXIT_SUCCESS;
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/
static int publishToTopicWithQos(MQTTClientSession_t *pMqttClientSess, MQTTClientCMD_t *pMqttCommand,
                                 PublishPackets_t *pOutgoingPublishPackets)
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    uint8_t publishIndex = MAX_OUTGOING_PUBLISHES;
#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
#endif

    assert(pMqttClientSess != NULL);
    assert(pMqttCommand != NULL);
    MQTTContext_t *pMqttContext = &pMqttClientSess->mqttContext;
    /* Get the next free index for the outgoing publish. All QoS2 outgoing
     * publishes are stored until a PUBREC is received. These messages are
     * stored for supporting a resend if a network connection is broken before
     * receiving a PUBREC. */
    returnStatus = getNextFreeIndexForOutgoingPublishes(pOutgoingPublishPackets, &publishIndex);

    if (returnStatus == EXIT_FAILURE) {
        MQTT_CLIENT_PRINTF("Unable to find a free spot for outgoing PUBLISH message.\n");
    } else {
        memcpy(&pOutgoingPublishPackets[publishIndex].pubInfo, &pMqttCommand->mqtt_cmd.publish,
               sizeof(MQTTPublishInfo_t));

        /* Get a new packet id. */
        pOutgoingPublishPackets[publishIndex].packetId = MQTT_GetPacketId(pMqttContext);
        pMqttClientSess->publishPacketIdentifier = pOutgoingPublishPackets[publishIndex].packetId;
        /* Send PUBLISH packet. */
        mqttStatus = MQTT_Publish(pMqttContext, &pOutgoingPublishPackets[publishIndex].pubInfo,
                                  pOutgoingPublishPackets[publishIndex].packetId);

        if (mqttStatus != MQTTSuccess) {
            MQTT_CLIENT_PRINTF("Failed to send PUBLISH packet to broker with error = %s.\n",
                               MQTT_Status_strerror(mqttStatus));
            cleanupOutgoingPublishAt(pOutgoingPublishPackets, publishIndex);
            returnStatus = EXIT_FAILURE;
        } else {
            MQTT_CLIENT_PRINTF("PUBLISH sent for topic %s to broker with packet ID %u.\n",
                               pOutgoingPublishPackets[publishIndex].pubInfo.pTopicName,
                               pOutgoingPublishPackets[publishIndex].packetId);
        }
    }

    if (returnStatus == EXIT_SUCCESS) {
        /* Process incoming packet from the broker. Acknowledgment for subscription
         * ( SUBACK ) will be received here. However after sending the subscribe, the
         * client may receive a publish before it receives a subscribe ack. Since this
         * demo is subscribing to the topic to which no one is publishing, probability
         * of receiving publish message before subscribe ack is zero; but application
         * must be ready to receive any packet. This demo uses MQTT_ProcessLoop to
         * receive packet from network. */
        returnStatus =
            waitForPacketAck(pMqttClientSess, pMqttClientSess->publishPacketIdentifier, MQTT_PROCESS_LOOP_TIMEOUT_MS);
    }

    total_send_len_one_raw += pMqttCommand->mqtt_cmd.publish.payloadLength;
    if (returnStatus == EXIT_SUCCESS) {
#ifdef CONFIG_QAT_MQTT_DEMO

        snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_PUBSUC:%d,%d,%d,\"%s\",%d", pMqttClientSess->sessionIndex,
                 total_send_len_one_raw, pMqttClientSess->mqttTransportScheme, pMqttClientSess->serverInfo.pHostName,
                 pMqttClientSess->serverInfo.port);
        QAT_Response_Str(QAT_RC_QUIET, buffer);

#endif
        MQTT_CLIENT_PRINTF("Publish topic success.\n");
    } else {
#ifdef CONFIG_QAT_MQTT_DEMO
        snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_PUBFAIL:%d,%d,\"%s\",%d", pMqttClientSess->sessionIndex,
                 pMqttClientSess->mqttTransportScheme, pMqttClientSess->serverInfo.pHostName,
                 pMqttClientSess->serverInfo.port);
        QAT_Response_Str(QAT_RC_QUIET, buffer);
#endif
        MQTT_CLIENT_PRINTF("Publish to the topic %s fail.\n", pOutgoingPublishPackets[publishIndex].pubInfo.pTopicName);
    }

    total_send_len_one_raw = 0;
    memset(pMqttCommand, 0, sizeof(MQTTClientCMD_t));
    return returnStatus;
}

/*-----------------------------------------------------------*/

static int initializeMqtt(MQTTClientSession_t *pMqttClientSess)
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;
    MQTTFixedBuffer_t networkBuffer;
    TransportInterface_t transport = {NULL};

    assert(pMqttClientSess != NULL);
    NetworkContext_t *pNetworkContext = &pMqttClientSess->networkContext;
    MQTTContext_t *pMqttContext = &pMqttClientSess->mqttContext;

    /* Set the pParams member of the network context with desired transport.
     * Fill in TransportInterface send and receive function pointers.
     * For this demo, TCP sockets are used to send and receive data
     * from network. Network context is SSL context for mbedtls.*/
    if (pMqttClientSess->mqttTransportScheme == MQTT_OVER_TCP) {
        pNetworkContext->pParams = &pMqttClientSess->plaintextParams;

        transport.pNetworkContext = pNetworkContext;
        transport.send = Plaintext_Send;
        transport.recv = Plaintext_Recv;
        transport.writev = NULL;

    } else if (pMqttClientSess->mqttTransportScheme == MQTT_OVER_SSL) {
        /* Set the pParams member of the network context with desired transport. */
        pNetworkContext->pParams = &(pMqttClientSess->tlsContext);

        transport.pNetworkContext = pNetworkContext;
        transport.send = TLS_FreeRTOS_send;
        transport.recv = TLS_FreeRTOS_recv;
        transport.writev = NULL;
    } else {
        assert(0);
    }

    /* Fill the values for network buffer. */
    networkBuffer.pBuffer = pMqttClientSess->buffer;
    networkBuffer.size = NETWORK_BUFFER_SIZE;

    /* Initialize MQTT library. */
    mqttStatus = MQTT_Init(pMqttContext, &transport, getTimeStampMs, eventCallback, &networkBuffer);

    if (mqttStatus != MQTTSuccess) {
        returnStatus = EXIT_FAILURE;
        MQTT_CLIENT_PRINTF("MQTT_Init failed: Status = %s.\n", MQTT_Status_strerror(mqttStatus));
    } else {
        mqttStatus =
            MQTT_InitStatefulQoS(pMqttContext, pMqttClientSess->pOutgoingPublishRecords, OUTGOING_PUBLISH_RECORD_LEN,
                                 pMqttClientSess->pIncomingPublishRecords, INCOMING_PUBLISH_RECORD_LEN);

        if (mqttStatus != MQTTSuccess) {
            returnStatus = EXIT_FAILURE;
            MQTT_CLIENT_PRINTF("MQTT_InitStatefulQoS failed: Status = %s.\n", MQTT_Status_strerror(mqttStatus));
        }
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int subscribeToTopic(MQTTClientSession_t *pMqttClientSess, MQTTClientCMD_t *pMqttCommand)
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    assert(pMqttClientSess != NULL);
    assert(pMqttCommand != NULL);
    MQTTContext_t *pMqttContext = &pMqttClientSess->mqttContext;

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
#endif

#ifdef CONFIG_QAT_MQTT_DEMO
    for (uint16_t i = 0; i < MQTT_SUB_TOPIC_PER_SESSION_MAX; i++) {
        if (strncmp(pMqttClientSess->subscribeInfo[i].pTopicFilter, pMqttCommand->mqtt_cmd.subscribe.pTopicFilter,
                    pMqttCommand->mqtt_cmd.subscribe.topicFilterLength) == 0) {
            snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_SUBALREADY:\"%s\"\n",
                     pMqttCommand->mqtt_cmd.subscribe.pTopicFilter);
            QAT_Response_Str(QAT_RC_QUIET, buffer);
            if (pMqttCommand->mqtt_cmd.subscribe.pTopicFilter) {
                free((char *)pMqttCommand->mqtt_cmd.subscribe.pTopicFilter);
            }

            memset(pMqttCommand, 0, sizeof(MQTTClientCMD_t));

            return returnStatus;
        }
    }
#endif
    MQTT_CLIENT_PRINTF("Subscribing to the MQTT topic %s.\n", pMqttCommand->mqtt_cmd.subscribe.pTopicFilter);
    /* Generate packet identifier for the SUBSCRIBE packet. */
    pMqttClientSess->subscribePacketIdentifier = MQTT_GetPacketId(pMqttContext);
    pMqttClientSess->subAckStatus = MQTTSubAckFailure;

    /* Send SUBSCRIBE packet. */
    mqttStatus =
        MQTT_Subscribe(pMqttContext, &pMqttCommand->mqtt_cmd.subscribe, 1, pMqttClientSess->subscribePacketIdentifier);

    if (mqttStatus != MQTTSuccess) {
        MQTT_CLIENT_PRINTF("Failed to send subscribe packet to broker with error = %s.\n",
                           MQTT_Status_strerror(mqttStatus));
        returnStatus = EXIT_FAILURE;
    } else {
        MQTT_CLIENT_DEBUG_PRINTF("Subscribe sent for topic %s to broker.\n",
                                 pMqttCommand->mqtt_cmd.subscribe.pTopicFilter);
    }

    if (returnStatus == EXIT_SUCCESS) {
        /* Process incoming packet from the broker. Acknowledgment for subscription
         * ( SUBACK ) will be received here. However after sending the subscribe, the
         * client may receive a publish before it receives a subscribe ack. Since this
         * demo is subscribing to the topic to which no one is publishing, probability
         * of receiving publish message before subscribe ack is zero; but application
         * must be ready to receive any packet. This demo uses MQTT_ProcessLoop to
         * receive packet from network. */
        returnStatus =
            waitForPacketAck(pMqttClientSess, pMqttClientSess->subscribePacketIdentifier, MQTT_PROCESS_LOOP_TIMEOUT_MS);
    }

    if (returnStatus == EXIT_SUCCESS) {
        /* Check if recent subscribe request has been rejected. subAckStatus is updated
         * in eventCallback to reflect the status of the SUBACK sent by the broker. */
        if (pMqttClientSess->subAckStatus == MQTTSubAckFailure) {
#ifdef CONFIG_QAT_MQTT_DEMO
            snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_SUBFAIL:%d,\"%s\"", pMqttClientSess->sessionIndex,
                     pMqttCommand->mqtt_cmd.subscribe.pTopicFilter);
            QAT_Response_Str(QAT_RC_QUIET, buffer);
#endif
            MQTT_CLIENT_PRINTF("Server rejected subscribe request to topic %s.\n",
                               pMqttCommand->mqtt_cmd.subscribe.pTopicFilter);

            returnStatus = EXIT_FAILURE;
        } else {
            /* Check status of the subscription request. If subAckStatus does not indicate
             * server refusal of the request (MQTTSubAckFailure), it contains the QoS level granted
             * by the server, indicating a successful subscription attempt. */
            MQTT_CLIENT_PRINTF("Subscribed to the topic %s. with maximum QoS %u success.\n",
                               pMqttCommand->mqtt_cmd.subscribe.pTopicFilter, pMqttClientSess->subAckStatus);
#ifdef CONFIG_QAT_MQTT_DEMO
            snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_SUBSUC:%d,\"%s\"", pMqttClientSess->sessionIndex,
                     pMqttCommand->mqtt_cmd.subscribe.pTopicFilter);
            QAT_Response_Str(QAT_RC_QUIET, buffer);
#endif

            int sub_index = -1;
            for (uint16_t i = 0; i < MQTT_SUB_TOPIC_PER_SESSION_MAX; i++) {
                if (pMqttClientSess->subscribeInfo[i].topicFilterLength == 0) {
                    sub_index = i;
                    break;
                }
            }
            if (sub_index != -1 && sub_index < MQTT_SUB_TOPIC_PER_SESSION_MAX) {
                pMqttClientSess->subscribeInfo[sub_index].qos = (MQTTQoS_t)pMqttClientSess->subAckStatus;
                pMqttClientSess->subscribeInfo[sub_index].pTopicFilter =
                    malloc(pMqttCommand->mqtt_cmd.subscribe.topicFilterLength + 1);
                memcpy((char *)(pMqttClientSess->subscribeInfo[sub_index].pTopicFilter),
                       pMqttCommand->mqtt_cmd.subscribe.pTopicFilter,
                       pMqttCommand->mqtt_cmd.subscribe.topicFilterLength + 1);
                pMqttClientSess->subscribeInfo[sub_index].topicFilterLength =
                    pMqttCommand->mqtt_cmd.subscribe.topicFilterLength;
            }
        }
    } else {
#ifdef CONFIG_QAT_MQTT_DEMO
        snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_SUBFAIL:%d,\"%s\"", pMqttClientSess->sessionIndex,
                 pMqttCommand->mqtt_cmd.subscribe.pTopicFilter);
        QAT_Response_Str(QAT_RC_QUIET, buffer);
#endif
        MQTT_CLIENT_PRINTF("Subscribed to the topic %s failed.\n", pMqttCommand->mqtt_cmd.subscribe.pTopicFilter);
    }

    if (pMqttCommand->mqtt_cmd.subscribe.pTopicFilter) {
        free((char *)pMqttCommand->mqtt_cmd.subscribe.pTopicFilter);
    }

    memset(pMqttCommand, 0, sizeof(MQTTClientCMD_t));
    return returnStatus;
}
/*-----------------------------------------------------------*/

static int unsubscribeFromTopic(MQTTClientSession_t *pMqttClientSess, MQTTClientCMD_t *pMqttCommand)
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
#endif

    assert(pMqttClientSess != NULL);
    assert(pMqttCommand != NULL);
    MQTTContext_t *pMqttContext = &pMqttClientSess->mqttContext;

    /* Generate packet identifier for the UNSUBSCRIBE packet. */
    pMqttClientSess->unsubscribePacketIdentifier = MQTT_GetPacketId(pMqttContext);
    MQTT_CLIENT_PRINTF("Unsubscribing to the MQTT topic %s.\n", pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter);

    /* Send UNSUBSCRIBE packet. */
    mqttStatus = MQTT_Unsubscribe(pMqttContext, &pMqttCommand->mqtt_cmd.unsubscribe, 1,
                                  pMqttClientSess->unsubscribePacketIdentifier);

    if (mqttStatus != MQTTSuccess) {
        MQTT_CLIENT_PRINTF("Failed to send UNSUBSCRIBE packet to broker with error = %s.\n",
                           MQTT_Status_strerror(mqttStatus));
        returnStatus = EXIT_FAILURE;
    } else {
        MQTT_CLIENT_DEBUG_PRINTF("UNSUBSCRIBE sent for topic %s to broker.\n",
                                 pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter);
    }

    if (returnStatus == EXIT_SUCCESS) {
        /* Process incoming packet from the broker. Acknowledgment for subscription
         * ( SUBACK ) will be received here. However after sending the subscribe, the
         * client may receive a publish before it receives a subscribe ack. Since this
         * demo is subscribing to the topic to which no one is publishing, probability
         * of receiving publish message before subscribe ack is zero; but application
         * must be ready to receive any packet. This demo uses MQTT_ProcessLoop to
         * receive packet from network. */
        returnStatus = waitForPacketAck(pMqttClientSess, pMqttClientSess->unsubscribePacketIdentifier,
                                        MQTT_PROCESS_LOOP_TIMEOUT_MS);
    }

    if (returnStatus == EXIT_SUCCESS) {
#ifdef CONFIG_QAT_MQTT_DEMO
        snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_UNSUBSUC:%d,\"%s\"", pMqttClientSess->sessionIndex,
                 pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter);
        QAT_Response_Str(QAT_RC_QUIET, buffer);
#endif
        MQTT_CLIENT_PRINTF("Unsubscribed from the topic %s success.\n",
                           pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter);

        int sub_index = -1;
        for (uint16_t i = 0; i < MQTT_SUB_TOPIC_PER_SESSION_MAX; i++) {
            if (strncmp(pMqttClientSess->subscribeInfo[i].pTopicFilter, pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter,
                        pMqttClientSess->subscribeInfo[i].topicFilterLength) == 0) {
                sub_index = i;
                break;
            }
        }
        if (sub_index != -1 && sub_index < MQTT_SUB_TOPIC_PER_SESSION_MAX &&
            pMqttClientSess->subscribeInfo[sub_index].pTopicFilter) {
            pMqttClientSess->subscribeInfo[sub_index].topicFilterLength = 0;
            free((char *)pMqttClientSess->subscribeInfo[sub_index].pTopicFilter);
            pMqttClientSess->subscribeInfo[sub_index].pTopicFilter = NULL;
        }
    } else {
#ifdef CONFIG_QAT_MQTT_DEMO
        snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_UNSUBFAIL:%d,\"%s\"", pMqttClientSess->sessionIndex,
                 pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter);
        QAT_Response_Str(QAT_RC_QUIET, buffer);
#endif
        MQTT_CLIENT_PRINTF("Unsubscribed from the topic %s fail.\n", pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter);
    }

    if (pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter) {
        free((char *)pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter);
    }

    memset(pMqttCommand, 0, sizeof(MQTTClientCMD_t));

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int disconnectMqttSession(MQTTContext_t *pMqttContext, MQTTClientSession_t *pMqttClientSess)
{
    MQTTStatus_t mqttStatus = MQTTSuccess;
    int returnStatus = EXIT_SUCCESS;

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
#endif
    assert(pMqttContext != NULL);

    /* Send DISCONNECT. */
    mqttStatus = MQTT_Disconnect(pMqttContext);

    if (mqttStatus != MQTTSuccess) {
        MQTT_CLIENT_PRINTF("Sending MQTT DISCONNECT failed with status=%s.\n", MQTT_Status_strerror(mqttStatus));
        returnStatus = EXIT_FAILURE;
    } else {
#ifdef CONFIG_QAT_MQTT_DEMO
        snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_DISCONNECTED:%d", pMqttClientSess->sessionIndex);
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        pMqttClientSess->mqttState = MQTT_DISCONNECT;
#endif
    }

    return returnStatus;
}

void mqtt_client_process_cmd(uint32_t sessionIndex)
{
    MQTTClientCMD_t *pMqttCommand = &mqtt_client_cmd[sessionIndex];
    MQTTContext_t *pMqttContext = &mqtt_client_sess[sessionIndex].mqttContext;
    MQTTClientSession_t *pMqttClientSess = &mqtt_client_sess[sessionIndex];
    MQTTTaskCtrl_t *pMqttTaskCtrl = &mqtt_task_ctrl;

    uint8_t index = 0;
    uint8_t outgoing_existed = 0;
    qbool_t isAllSessionDisconn = true;

    if (pMqttClientSess->mqttState == MQTT_CONNECTED) {
        for (index = 0; index < MAX_OUTGOING_PUBLISHES; index++) {
            if (pMqttClientSess->outgoingPublishPackets[index].packetId != 0)
                outgoing_existed = 1;
        }
        if (outgoing_existed == 1) {
            handlePublishResend(pMqttClientSess->outgoingPublishPackets, pMqttContext);
        }
    }

    switch (pMqttCommand->cmd_type) {
        case MQTT_CMD_PUB:
            if (pMqttClientSess->mqttState == MQTT_CONNECTED) {
                if (pMqttCommand->mqtt_cmd.publish.qos == MQTTQoS0) {
                    publishToTopic(pMqttContext, pMqttCommand, pMqttClientSess);
                    qurt_signal_set(&pMqttTaskCtrl->mqtt_client_signal, MQTT_PUB_RAW_DONE);
                } else {
                    publishToTopicWithQos(pMqttClientSess, pMqttCommand, pMqttClientSess->outgoingPublishPackets);
                    qurt_signal_set(&pMqttTaskCtrl->mqtt_client_signal, MQTT_PUB_RAW_DONE);
                }
            }

            break;

        case MQTT_CMD_SUB:
            if (pMqttClientSess->mqttState == MQTT_CONNECTED) {
                subscribeToTopic(pMqttClientSess, pMqttCommand);
            }

            break;

        case MQTT_CMD_UNSUB:
            if (pMqttClientSess->mqttState == MQTT_CONNECTED) {
                unsubscribeFromTopic(pMqttClientSess, pMqttCommand);
            }

            break;

        case MQTT_CMD_DISC:
            if (pMqttClientSess->mqttState == MQTT_CONNECTED) {
                disconnectMqttSession(&pMqttClientSess->mqttContext, pMqttClientSess);
            }

            // qurt_thread_sleep(10);
            pMqttClientSess->mqttState = MQTT_FORCE_DISCONNECT;

            /* End TLS session, then close TCP connection. */
            if (pMqttClientSess->mqttTransportScheme == MQTT_OVER_TCP) {
                Plaintext_Disconnect(&pMqttClientSess->networkContext);
            } else if (pMqttClientSess->mqttTransportScheme == MQTT_OVER_SSL) {
                TLS_FreeRTOS_Disconnect(&pMqttClientSess->networkContext);
            } else {
                assert(0);
            }

            /* clear command */
            memset(pMqttCommand, 0, sizeof(MQTTClientCMD_t));
            /* Log message indicating disconnect successfully. */
            MQTT_CLIENT_PRINTF("MQTT session:%d disconnect successfully.\n", pMqttClientSess->sessionIndex);

            for (uint32 index = 0; index < MQTT_DEMO_SESSION_NUM; index++) {
                if (mqtt_client_sess[index].mqttState == MQTT_CONNECTED) {
                    isAllSessionDisconn = false;
                    break;
                }
            }
            if (isAllSessionDisconn) {
                mqttRxThreadCreated = false;
            }
            break;

        default:
            break;
    }
}
void mqttc_rx_task(void __attribute__((__unused__)) * pvParameters)
{
    uint32 sessionIndex;
    MQTTClientSession_t *pMqttClientSess;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    PlaintextParams_t *pPlaintextParams = NULL;
    NetworkContext_t *pNetworkContext = NULL;
    MQTTTaskCtrl_t *pMqttTaskCtrl = &mqtt_task_ctrl;
    int32_t selectStatus = 1;
    uint8_t state_prv = 0, state_connected = 0;
    int prev_fd = 0, max_fd = 0;
    fd_set read_fds;
    memset(&read_fds, 0, sizeof(read_fds));

    do {
        for (sessionIndex = 0; sessionIndex < MQTT_DEMO_SESSION_NUM; sessionIndex++) {
            pMqttClientSess = &mqtt_client_sess[sessionIndex];
            pNetworkContext = pMqttClientSess->mqttContext.transportInterface.pNetworkContext;

            if (pMqttClientSess->mqttState == MQTT_CONNECTED) {
                // assert( pNetworkContext != NULL && pNetworkContext->pParams != NULL );
                pPlaintextParams = pNetworkContext->pParams;
                /* Initialize the file descriptor. */

                /* Set the file descriptor for select. */
                FD_SET(pPlaintextParams->socketDescriptor, &read_fds);

                max_fd = pPlaintextParams->socketDescriptor > prev_fd ? pPlaintextParams->socketDescriptor : prev_fd;

                prev_fd = pPlaintextParams->socketDescriptor;

                state_connected = 1;
            }
        }
        if (state_connected) {
            selectStatus = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
            qurt_signal_set(&pMqttTaskCtrl->mqtt_client_signal, MQTT_PKT_RECV);
        }
        max_fd = 0;
        prev_fd = 0;
        state_connected = 0;
        memset(&read_fds, 0, sizeof(read_fds));

        if (mqttRxThreadCreated == false) {
            nt_osal_thread_delete(NULL);
        }
    } while (1);
}
void mqttc_task(void __attribute__((__unused__)) * pvParameters)
{
    uint32 sessionIndex;
    bool delThread;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    uint8_t recover_from_disconn = 0;

    MQTTClientSession_t *pMqttClientSess;
    MQTTTaskCtrl_t *pMqttTaskCtrl = &mqtt_task_ctrl;

    uint32_t signal = 0;
#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
#endif

    do {
        signal = qurt_signal_wait(
            &pMqttTaskCtrl->mqtt_client_signal,
            MQTT_PUB_RAW_START | MQTT_PUB_KEEPALIVE | MQTT_SUB | MQTT_UNSUB | MQTT_DISCONN | MQTT_PKT_RECV,
            QURT_SIGNAL_ATTR_CLEAR_MASK | QURT_SIGNAL_ATTR_WAIT_ANY);

        for (sessionIndex = 0; sessionIndex < MQTT_DEMO_SESSION_NUM; sessionIndex++) {
            pMqttClientSess = &mqtt_client_sess[sessionIndex];

            recover_from_disconn = 0;

            if (pMqttClientSess->mqttState == MQTT_CONNECTED) {
                mqttStatus = MQTT_ProcessLoop(&pMqttClientSess->mqttContext);

                if (mqttStatus == MQTTKeepAliveTimeout) {
#ifdef CONFIG_QAT_MQTT_DEMO
                    snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_KEEPLIVETIMEOUT:%d\n",
                             pMqttClientSess->sessionIndex);
                    QAT_Response_Str(QAT_RC_QUIET, buffer);
#else
                    MQTT_CLIENT_PRINTF("MQTT session:%d MQTT keep alive timeout\n", pMqttClientSess->sessionIndex);
#endif
                    pMqttClientSess->mqttContext.waitingForPingResp = false;

                    uint32_t now = getTimeStampMs();

                    if ((now - pMqttClientSess->mqttContext.lastPacketRxTime >
                         MQTT_PING_RESP_TIMEOUT_MAX_TIMES * pMqttClientSess->connectInfo.keepAliveSeconds) &&
                        (now - pMqttClientSess->mqttContext.lastPacketRxTime < 0xFFFFFF)) {
                        MQTT_CLIENT_PRINTF("MQTT session:%d PING RESP timeout %d times, disconnect with the broker.\n",
                                           pMqttClientSess->sessionIndex, MQTT_PING_RESP_TIMEOUT_MAX_TIMES);
                        disconnectMqttSession(&pMqttClientSess->mqttContext, pMqttClientSess);
                        pMqttClientSess->mqttState = MQTT_DISCONNECT;
                        // qurt_thread_sleep(10);
                    }
                } else if (mqttStatus == MQTTBadResponse) {
                    MQTT_CLIENT_PRINTF("MQTT session:%d receive unknown packet type.\n", pMqttClientSess->sessionIndex);
                    /* Clear the buffer */
                    (void)memset(pMqttClientSess->mqttContext.networkBuffer.pBuffer, 0,
                                 pMqttClientSess->mqttContext.networkBuffer.size);

                    /* Reset the index. */
                    pMqttClientSess->mqttContext.index = 0;
                }

                if ((mqttStatus == MQTTRecvFailed) || (mqttStatus == MQTTSendFailed)) {
                    MQTT_CLIENT_PRINTF(
                        "MQTT session:%d MQTT_ProcessLoop returned with status = %s disconnect with broker\n",
                        pMqttClientSess->sessionIndex, MQTT_Status_strerror(mqttStatus));
                    disconnectMqttSession(&pMqttClientSess->mqttContext, pMqttClientSess);
                    // qurt_thread_sleep(10);
                    pMqttClientSess->mqttState = MQTT_DISCONNECT;
                }
            }

            if (pMqttClientSess->mqttState == MQTT_DISCONNECT) {
                int returnStatus = EXIT_SUCCESS;

                /* End TLS session, then close TCP connection. */
                if (pMqttClientSess->mqttTransportScheme == MQTT_OVER_TCP) {
                    Plaintext_Disconnect(&pMqttClientSess->networkContext);
                } else if (pMqttClientSess->mqttTransportScheme == MQTT_OVER_SSL) {
                    TLS_FreeRTOS_Disconnect(&pMqttClientSess->networkContext);
                } else {
                    assert(0);
                }

                returnStatus = connectToServerWithBackoffRetries(pMqttClientSess, 1);

                if (returnStatus == EXIT_SUCCESS) {
                    pMqttClientSess->mqttState = MQTT_CONNECTED;
                    recover_from_disconn = 1;
#ifdef CONFIG_QAT_MQTT_DEMO
                    snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_CONNECTED:%d,%d,\"%s\",%d,%d",
                             pMqttClientSess->sessionIndex, pMqttClientSess->mqttTransportScheme,
                             pMqttClientSess->serverInfo.pHostName, pMqttClientSess->serverInfo.port, 1);
                    QAT_Response_Str(QAT_RC_QUIET, buffer);
#endif
                }
            }

            if (recover_from_disconn == 0) {
                mqtt_client_process_cmd(sessionIndex);
            }
        }

        delThread = true;
        for (uint32 index = 0; index < MQTT_DEMO_SESSION_NUM; index++) {
            if (mqtt_client_sess[index].mqttState != MQTT_INIT) {
                delThread = false;
                break;
            }
        }

        if (delThread == true) {
            mqttThreadCreated = false;
            nt_osal_thread_delete(NULL);
        }
    } while (1);
}

static void mqtt_client_help()
{
    MQTT_CLIENT_PRINTF("Usage:\n");
    MQTT_CLIENT_PRINTF(
        "mqttc init <session_id> [-s <transport_scheme>] [--ca <file>] [--cert <file>] [--key <file>] [--sni] [--alpn "
        "<protocol_name>]\n");
    MQTT_CLIENT_PRINTF(
        "mqttc connect <session_id> -h <host> [-p <port>] [-u <username> -P <password>] [-i <client_id>] [-k "
        "<keepalive>] [-c] &\n");
    MQTT_CLIENT_PRINTF("mqttc publish <session_id> -t <topic> [-q <qos_level>] -m <message> [-r]\n");
    MQTT_CLIENT_PRINTF("mqttc subscribe <session_id> -t <topic> [-q <qos_level>]\n");
    MQTT_CLIENT_PRINTF("mqttc unsubscribe <session_id> -t <topic>\n");
    MQTT_CLIENT_PRINTF("mqttc disconnect <session_id>\n");
    MQTT_CLIENT_PRINTF("mqttc destroy <session_id>\n");
    MQTT_CLIENT_PRINTF("options:\n");
    MQTT_CLIENT_PRINTF("<session_id> demo session :, can be 0 or 1.\n");
    MQTT_CLIENT_PRINTF("-s <transport_scheme> transport layer to use:ssl or tcp. Defaults to tcp.\n");
    MQTT_CLIENT_PRINTF(
        "--ca <file> file path containing trusted CA certificates to enable encrypted certificate. Defaults to "
        "null.\n");
    MQTT_CLIENT_PRINTF(
        "--cert <file> client certificate for authentication, if required by server. Defaults to null.\n");
    MQTT_CLIENT_PRINTF(
        "--key <file> client private key for authentication, if required by server. Defaults to null.\n");
    MQTT_CLIENT_PRINTF("--sni Server Name Indication enable. Defaults to false, SNI is same as hostname.\n");
    MQTT_CLIENT_PRINTF("--alpn <protocol_name> ApplicationLayerProtocol Negotiation. Defaults to null.\n");
    MQTT_CLIENT_PRINTF("-h <host> mqtt host to connect to.\n");
    MQTT_CLIENT_PRINTF("-p <port> network port to connect to. Defaults to 1883 for plain MQTT.\n");
    MQTT_CLIENT_PRINTF("-u <username> mqtt username. Defaults to null.\n");
    MQTT_CLIENT_PRINTF("-P <password> mqtt password Defaults to null.\n");
    MQTT_CLIENT_PRINTF("-i <client_id> id to use for this client. Defaults to testclient.\n");
    MQTT_CLIENT_PRINTF("-k <keepalive> keep alive in seconds for this client. Defaults to 60.\n");
    MQTT_CLIENT_PRINTF("-c clean session with the broker, Defaults to false.\n");
    MQTT_CLIENT_PRINTF("-t <topic> mqtt topic to publish/subscribe/unsubscribe to.\n");
    MQTT_CLIENT_PRINTF("-q <qos_level> quality of service level to use. Defaults to 0.\n");
    MQTT_CLIENT_PRINTF("-m <message> message payload to send.\n");
    MQTT_CLIENT_PRINTF("-r message should be retained, Defaults to false.\n");
    MQTT_CLIENT_PRINTF("& should be added to the end of connect command.\n");
}

void cleanupNetworkCredentials(MQTTClientSession_t *pMqttClientSess)
{
    assert(pMqttClientSess != NULL);

    pMqttClientSess->mqttTransportScheme = MQTT_OVER_TCP;

    pMqttClientSess->tlsCredentials.pAlpnProtos = NULL;

    if (pMqttClientSess->tlsCredentials.pcAlpnProtocols[0] != NULL) {
        free((void *)pMqttClientSess->tlsCredentials.pcAlpnProtocols[0]);
        pMqttClientSess->tlsCredentials.pcAlpnProtocols[0] = NULL;
        pMqttClientSess->tlsCredentials.pAlpnProtos = NULL;
    }

    pMqttClientSess->tlsCredentials.disableSni = pdFALSE;

    if (pMqttClientSess->tlsCredentials.pRootCa != NULL) {
        free((void *)pMqttClientSess->tlsCredentials.pRootCa);
        pMqttClientSess->tlsCredentials.pRootCa = NULL;
    }

    if (pMqttClientSess->tlsCredentials.pClientCert != NULL) {
        free((void *)pMqttClientSess->tlsCredentials.pClientCert);
        pMqttClientSess->tlsCredentials.pClientCert = NULL;
    }

    if (pMqttClientSess->tlsCredentials.pPrivateKey != NULL) {
        free((void *)pMqttClientSess->tlsCredentials.pPrivateKey);
        pMqttClientSess->tlsCredentials.pPrivateKey = NULL;
    }
}

void mqtt_keepalive_timer_cb(void)
{
    MQTTTaskCtrl_t *pMqttTaskCtrl = &mqtt_task_ctrl;

    qurt_signal_set(&pMqttTaskCtrl->mqtt_client_signal, MQTT_PUB_KEEPALIVE);

    if (nt_start_timer(pMqttTaskCtrl->mqtt_keepalive_timer) != NT_TIMER_SUCCESS) {
        MQTT_CLIENT_PRINTF("QAT MQTT keepalive timer start failed\n");
    }
}

qapi_Status_t mqttc_init(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t index = 1;
    uint32_t ret_val = 0;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
    index = 0;
#endif

#ifdef CONFIG_QAT_MQTT_DEMO
    if (Parameter_Count < 1)
#else
    if (Parameter_Count < 2)
#endif
    {
        mqtt_client_help();
        return QAPI_ERR_INVALID_PARAM;
    }

    /*    [0]   [1]
     mqttc init <session_id> [-s <transport scheme>][--ca file] [--cert file] [--key file] [--sni] [--alpn
     <protocol_name>]
    */
    /*                 [0]   [1]
    +MQTTINIT=<session_id>,<transport scheme>,<ca_file>,<cert_file>,<key_file>,<sni>,<alpn protocol_name>
    */

    sessionIndex = Parameter_List[index].Integer_Value;
    isIntegerValid = Parameter_List[index].Integer_Is_Valid;

    if (isIntegerValid == false) {
        MQTT_CLIENT_PRINTF("Invalid session id %s\n", Parameter_List[index].String_Value);
        goto end;
    }

    if (sessionIndex > MQTT_DEMO_SESSION_NUM) {
        MQTT_CLIENT_PRINTF("Invalid session id %d\n", sessionIndex);
        goto end;
    }

    index++;
    MQTTClientSession_t *pMqttClientSess = &mqtt_client_sess[sessionIndex];
    MQTTTaskCtrl_t *pMqttTaskCtrl = &mqtt_task_ctrl;

    if ((pMqttClientSess->mqttState != MQTT_INIT) && pMqttClientSess->mqttState != MQTT_FORCE_DISCONNECT) {
        MQTT_CLIENT_PRINTF("MQTT state:%d, execute disconnect command.\n", pMqttClientSess->mqttState);
        goto end;
    }

    pMqttClientSess->sessionIndex = sessionIndex;
    cleanupNetworkCredentials(pMqttClientSess);

    while (index < Parameter_Count) {
#ifdef CONFIG_QAT_MQTT_DEMO
        // <transport scheme>
        if (1 == index)
#else
        if (0 == strcmp(Parameter_List[index].String_Value, "-s"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif

            if (strcasecmp(Parameter_List[index].String_Value, "ssl") == 0) {
                pMqttClientSess->mqttTransportScheme = MQTT_OVER_SSL;
            } else if (strcasecmp(Parameter_List[index].String_Value, "tcp") == 0) {
                pMqttClientSess->mqttTransportScheme = MQTT_OVER_TCP;
            } else {
                MQTT_CLIENT_PRINTF("Invalid -s parameter : %s\n", Parameter_List[index].String_Value);
                goto fail;
            }

            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        // <ca_file>
        else if (2 == index)
#else
        else if (0 == strcasecmp(Parameter_List[index].String_Value, "--ca"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif

            const char *filename = Parameter_List[index].String_Value;
            int rootCaLen = 0;
#ifdef CONFIG_FILE_SYSTEM
            int file = open(filename, O_RDONLY, 0);

            if (file < 0) {
                MQTT_CLIENT_PRINTF("open root CA : %s fail.\n", filename);
                goto fail;
            }

            rootCaLen = lseek(file, 0, SEEK_END);

            if (rootCaLen <= 0) {
                MQTT_CLIENT_PRINTF("Error root CA : %s len:%d.\n", filename, rootCaLen);
                close(file);
                goto fail;
            }

            uint8_t *rootCa = malloc(rootCaLen + 1);

            if (rootCa == NULL) {
                MQTT_CLIENT_PRINTF("root CA malloc len:%d fail.\n", rootCaLen);
                close(file);
                goto fail;
            }

            pMqttClientSess->tlsCredentials.pRootCa = rootCa;

            lseek(file, 0, SEEK_SET);

            read(file, rootCa, rootCaLen);
            close(file);
            rootCa[rootCaLen] = 0;

            pMqttClientSess->tlsCredentials.rootCaSize = rootCaLen + 1;
#endif
            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        // <cert_file>
        else if (3 == index)
#else
        else if (0 == strcasecmp(Parameter_List[index].String_Value, "--cert"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif

            const char *filename = Parameter_List[index].String_Value;
            int clientCertLen = 0;
#ifdef CONFIG_FILE_SYSTEM
            int file = open(filename, O_RDONLY, 0);

            if (file < 0) {
                MQTT_CLIENT_PRINTF("open client cert : %s fail.\n", filename);
                goto fail;
            }

            clientCertLen = lseek(file, 0, SEEK_END);

            if (clientCertLen <= 0) {
                MQTT_CLIENT_PRINTF("Error client cert : %s len:%d.\n", filename, clientCertLen);
                close(file);
                goto fail;
            }

            uint8_t *clientCert = malloc(clientCertLen + 1);

            if (clientCert == NULL) {
                MQTT_CLIENT_PRINTF("client cert malloc len:%d fail.\n", clientCertLen);
                close(file);
                goto fail;
            }

            pMqttClientSess->tlsCredentials.pClientCert = clientCert;

            lseek(file, 0, SEEK_SET);

            read(file, clientCert, clientCertLen);

            clientCert[clientCertLen] = 0;
            close(file);

            pMqttClientSess->tlsCredentials.clientCertSize = clientCertLen + 1;

#endif
            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        // <key_file>
        else if (4 == index)
#else
        else if (0 == strcasecmp(Parameter_List[index].String_Value, "--key"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif

            const char *filename = Parameter_List[index].String_Value;
            int clientKeyLen = 0;
#ifdef CONFIG_FILE_SYSTEM
            int file = open(filename, O_RDONLY, 0);

            if (file < 0) {
                MQTT_CLIENT_PRINTF("open client key : %s fail.\n", filename);
                goto fail;
            }

            clientKeyLen = lseek(file, 0, SEEK_END);

            if (clientKeyLen <= 0) {
                MQTT_CLIENT_PRINTF("Error client key : %s len:%d.\n", filename, clientKeyLen);
                close(file);
                goto fail;
            }

            uint8_t *clientKey = malloc(clientKeyLen + 1);

            if (clientKey == NULL) {
                MQTT_CLIENT_PRINTF("client key malloc len:%d fail.\n", clientKeyLen);
                close(file);
                goto fail;
            }

            pMqttClientSess->tlsCredentials.pPrivateKey = clientKey;

            lseek(file, 0, SEEK_SET);

            read(file, clientKey, clientKeyLen);
            close(file);

            clientKey[clientKeyLen] = 0;

            pMqttClientSess->tlsCredentials.privateKeySize = clientKeyLen + 1;
#endif
            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        // <sni>
        else if (5 == index)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "--sni"))
#endif
        {
            index++;
            pMqttClientSess->tlsCredentials.disableSni = Parameter_List[index].Integer_Value;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        // <alpn protocol_name>
        else if (6 == index)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "--alpn"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif
            pMqttClientSess->tlsCredentials.pcAlpnProtocols[0] = malloc(strlen(Parameter_List[index].String_Value) + 1);

            if (pMqttClientSess->tlsCredentials.pcAlpnProtocols[0] == NULL) {
                MQTT_CLIENT_PRINTF("ALPN malloc(%d) failed\n", strlen(Parameter_List[index].String_Value) + 1);
                goto fail;
            }

            memcpy((char *)pMqttClientSess->tlsCredentials.pcAlpnProtocols[0], Parameter_List[index].String_Value,
                   strlen(Parameter_List[index].String_Value) + 1);
            pMqttClientSess->tlsCredentials.pAlpnProtos = pMqttClientSess->tlsCredentials.pcAlpnProtocols;
            index++;
        } else {
            MQTT_CLIENT_PRINTF("Invalid parameter %s\n", Parameter_List[index].String_Value);
            index++;
            goto fail;
        }
    }

    if (pMqttClientSess->mqttTransportScheme == MQTT_OVER_SSL) {
        /*SSL mode must configure CA certificates */
        if (pMqttClientSess->tlsCredentials.pRootCa == NULL) {
            MQTT_CLIENT_PRINTF("MQTT session:%d init fail, SSL should configure CA certificates.\n",
                               pMqttClientSess->sessionIndex);
            goto fail;
        }
    }

    pMqttClientSess->mqttState = MQTT_INIT;

    if (pMqttTaskCtrl->mqtt_signal_created == false) {
        ret_val = qurt_signal_create(&pMqttTaskCtrl->mqtt_client_signal);
    }

    pMqttTaskCtrl->mqtt_signal_created = true;
#ifdef CONFIG_QAT_MQTT_DEMO
    snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_INITED:%d", pMqttClientSess->sessionIndex);
    QAT_Response_Str(QAT_RC_QUIET, buffer);
#else
    MQTT_CLIENT_PRINTF("MQTT session:%d init successfully.\n", pMqttClientSess->sessionIndex);
#endif

end:
    return QAPI_OK;
fail:
    cleanupNetworkCredentials(pMqttClientSess);

    return QAPI_ERR_INVALID_PARAM;
}
void cleanupConnectInfo(MQTTClientSession_t *pMqttClientSess)
{
    if (pMqttClientSess->serverInfo.pHostName != NULL) {
        free((char *)pMqttClientSess->serverInfo.pHostName);
        pMqttClientSess->serverInfo.pHostName = NULL;
    }

    pMqttClientSess->serverInfo.hostNameLength = 0;

    pMqttClientSess->serverInfo.port = 1884;

    /* Start with a clean session with false, establishing a connection with clean session
     * will ensure that the broker store any data when this client gets disconnected. */
    pMqttClientSess->connectInfo.cleanSession = false;

    /* The maximum time interval in seconds which is allowed to elapse
     * between two Control Packets.
     * It is the responsibility of the Client to ensure that the interval between
     * Control Packets being sent does not exceed the this Keep Alive value. In the
     * absence of sending any other Control Packets, the Client MUST send a
     * PINGREQ Packet. */
    pMqttClientSess->connectInfo.keepAliveSeconds = MQTT_KEEP_ALIVE_INTERVAL_SECONDS;

    if (pMqttClientSess->connectInfo.pClientIdentifier != NULL) {
        free((char *)pMqttClientSess->connectInfo.pClientIdentifier);
        pMqttClientSess->connectInfo.pClientIdentifier = NULL;
    }

    pMqttClientSess->connectInfo.clientIdentifierLength = 0;

    if (pMqttClientSess->connectInfo.pUserName != NULL) {
        free((char *)pMqttClientSess->connectInfo.pUserName);
        pMqttClientSess->connectInfo.pUserName = NULL;
    }

    pMqttClientSess->connectInfo.userNameLength = 0;

    if (pMqttClientSess->connectInfo.pPassword != NULL) {
        free((char *)pMqttClientSess->connectInfo.pPassword);
        pMqttClientSess->connectInfo.pPassword = NULL;
    }

    pMqttClientSess->connectInfo.passwordLength = 0;
}

qapi_Status_t mqttc_connect_info_query(void)
{
    MQTTClientSession_t *pMqttClientSess;
    uint16_t sessionIndex = 0;
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
    for (sessionIndex = 0; sessionIndex < MQTT_DEMO_SESSION_NUM; sessionIndex++) {
        pMqttClientSess = &mqtt_client_sess[sessionIndex];
#ifdef CONFIG_QAT_MQTT_DEMO

        snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+MQTTCONN:%d,%d,%d,\"%s\",%d", (uint16_t)sessionIndex,
                 pMqttClientSess->mqttState, pMqttClientSess->mqttTransportScheme,
                 pMqttClientSess->serverInfo.pHostName ? pMqttClientSess->serverInfo.pHostName : "NULL",
                 (uint16_t)pMqttClientSess->serverInfo.port);
        QAT_Response_Str(QAT_RC_QUIET, buffer);

#else

#endif
    }
}

qapi_Status_t mqttc_connect(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t index = 1;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    MQTTTaskCtrl_t *pMqttTaskCtrl = &mqtt_task_ctrl;

    if (Parameter_Count < 2) {
        mqtt_client_help();
        return QAPI_ERR_INVALID_PARAM;
    }

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
    index = 0;
#endif

    /*       [0]   [1]
    mqttc connect <session_id> -h <host> [-p <port>] [-u <username>] [-P <password>] [-k <keepalive>] [-i
    <clientid>][-c]
    */

    /*             [0]      [1]
    +MQTTCONN=<session_id>,<host>,<port>,<clientid>,<username>,<password>,<keepalive>,<clean session>
    */

    sessionIndex = Parameter_List[index].Integer_Value;
    isIntegerValid = Parameter_List[index].Integer_Is_Valid;

    if (isIntegerValid == false) {
        MQTT_CLIENT_PRINTF("Invalid session id %s\n", Parameter_List[index].String_Value);
        goto end;
    }

    if (sessionIndex > MQTT_DEMO_SESSION_NUM) {
        MQTT_CLIENT_PRINTF("Invalid session id %d\n", sessionIndex);
        goto end;
    }

    index++;
    MQTTClientSession_t *pMqttClientSess = &mqtt_client_sess[sessionIndex];
    pMqttClientSess->sessionIndex = sessionIndex;

    if ((pMqttClientSess->mqttState != MQTT_INIT) && pMqttClientSess->mqttState != MQTT_FORCE_DISCONNECT) {
        MQTT_CLIENT_PRINTF("MQTT state:%d, execute disconnect command.\n", pMqttClientSess->mqttState);
        goto end;
    }

    cleanupConnectInfo(pMqttClientSess);
    uint16_t keepAliveSeconds = 0;
    while (index < Parameter_Count) {
#ifdef CONFIG_QAT_MQTT_DEMO
        //  <host>
        if (index == 1)
#else
        if (0 == strcmp(Parameter_List[index].String_Value, "-h"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif
            pMqttClientSess->serverInfo.hostNameLength = strlen(Parameter_List[index].String_Value);

            pMqttClientSess->serverInfo.pHostName = malloc(pMqttClientSess->serverInfo.hostNameLength + 1);

            if (pMqttClientSess->serverInfo.pHostName == NULL) {
                MQTT_CLIENT_PRINTF("MQTT malloc hostname (%d) fail\n", pMqttClientSess->serverInfo.hostNameLength + 1);
                goto fail;
            }

            memcpy((char *)pMqttClientSess->serverInfo.pHostName, Parameter_List[index].String_Value,
                   pMqttClientSess->serverInfo.hostNameLength + 1);
            index++;

        }
#ifdef CONFIG_QAT_MQTT_DEMO
        // <port>
        else if (index == 2)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "-p"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif
            pMqttClientSess->serverInfo.port = Parameter_List[index].Integer_Value;
            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        // <username>
        else if (index == 4)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "-u"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif

            pMqttClientSess->connectInfo.userNameLength = strlen(Parameter_List[index].String_Value);

            pMqttClientSess->connectInfo.pUserName = malloc(pMqttClientSess->connectInfo.userNameLength + 1);

            if (pMqttClientSess->connectInfo.pUserName == NULL) {
                MQTT_CLIENT_PRINTF("MQTT malloc username (%d) fail\n", pMqttClientSess->connectInfo.userNameLength + 1);
                goto fail;
            }

            memcpy((char *)pMqttClientSess->connectInfo.pUserName, Parameter_List[index].String_Value,
                   pMqttClientSess->connectInfo.userNameLength + 1);

            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        // <password>
        else if (index == 5)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "-P"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif

            pMqttClientSess->connectInfo.passwordLength = strlen(Parameter_List[index].String_Value);

            pMqttClientSess->connectInfo.pPassword = malloc(pMqttClientSess->connectInfo.passwordLength + 1);

            if (pMqttClientSess->connectInfo.pPassword == NULL) {
                MQTT_CLIENT_PRINTF("MQTT malloc password (%d) fail\n", pMqttClientSess->connectInfo.passwordLength + 1);
                goto fail;
            }

            memcpy((char *)pMqttClientSess->connectInfo.pPassword, Parameter_List[index].String_Value,
                   pMqttClientSess->connectInfo.passwordLength + 1);

            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        // <clientid>
        else if (index == 3)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "-i"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif
            /* The client identifier is used to uniquely identify this MQTT client to
             * the MQTT broker. In a production device the identifier can be something
             * unique, such as a device serial number. */
            pMqttClientSess->connectInfo.clientIdentifierLength = strlen(Parameter_List[index].String_Value);

            pMqttClientSess->connectInfo.pClientIdentifier =
                malloc(pMqttClientSess->connectInfo.clientIdentifierLength + 1);

            if (pMqttClientSess->connectInfo.pClientIdentifier == NULL) {
                MQTT_CLIENT_PRINTF("MQTT malloc client id (%d) fail\n",
                                   pMqttClientSess->connectInfo.clientIdentifierLength + 1);
                goto fail;
            }

            memcpy((char *)pMqttClientSess->connectInfo.pClientIdentifier, Parameter_List[index].String_Value,
                   pMqttClientSess->connectInfo.clientIdentifierLength + 1);

            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        // <keepalive>
        else if (index == 6)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "-k"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif
            if (Parameter_List[index].Integer_Value <= 0) {
                MQTT_CLIENT_PRINTF("Value of keepalive must be a positive integer.\n");
                goto fail;
            }

            keepAliveSeconds = Parameter_List[index].Integer_Value;
            if ((PACKET_TX_TIMEOUT_MS / 1000 < keepAliveSeconds) || (PACKET_RX_TIMEOUT_MS / 1000 < keepAliveSeconds)) {
                MQTT_CLIENT_PRINTF(
                    "Value of keepalive has been limited to the minimum value of PACKET_TX_TIMEOUT_MS/1000 and "
                    "BPACKET_RX_TIMEOUT_MS/1000.\n");
                MQTT_CLIENT_PRINTF(
                    "PACKET_TX_TIMEOUT_MS and BPACKET_RX_TIMEOUT_MS can be modified in core_mqtt_config.h, both "
                    "default to 120000.\n");
            }

            pMqttClientSess->connectInfo.keepAliveSeconds = Parameter_List[index].Integer_Value;
            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        // <clean session>
        else if (index == 7)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "-c"))
#endif
        {
            index++;
            pMqttClientSess->connectInfo.cleanSession = true;
        } else {
            MQTT_CLIENT_PRINTF("Invalid parameter %s\n", Parameter_List[index].String_Value);
            index++;
            return QAPI_ERR_INVALID_PARAM;
        }
    }

    /* The client identifier is null, set default value. */
    if (pMqttClientSess->connectInfo.pClientIdentifier == NULL) {
        pMqttClientSess->connectInfo.clientIdentifierLength = CLIENT_IDENTIFIER_LENGTH;

        pMqttClientSess->connectInfo.pClientIdentifier =
            malloc(pMqttClientSess->connectInfo.clientIdentifierLength + 1);

        if (pMqttClientSess->connectInfo.pClientIdentifier == NULL) {
            MQTT_CLIENT_PRINTF("MQTT malloc defaut client id (%d) fail\n",
                               pMqttClientSess->connectInfo.clientIdentifierLength + 1);
            goto fail;
        }

        memcpy((char *)pMqttClientSess->connectInfo.pClientIdentifier, CLIENT_IDENTIFIER,
               pMqttClientSess->connectInfo.clientIdentifierLength + 1);
    }

    if (pMqttClientSess->serverInfo.pHostName == NULL) {
        MQTT_CLIENT_PRINTF("Error:hostname is not configured\n");
        goto fail;
    }

    /* LWT Info. */
    pMqttClientSess->lwtInfo.pTopicName = MQTT_EXAMPLE_TOPIC;
    pMqttClientSess->lwtInfo.topicNameLength = MQTT_EXAMPLE_TOPIC_LENGTH;
    pMqttClientSess->lwtInfo.pPayload = MQTT_EXAMPLE_MESSAGE;
    pMqttClientSess->lwtInfo.payloadLength = strlen(MQTT_EXAMPLE_MESSAGE);
    pMqttClientSess->lwtInfo.qos = MQTTQoS0;
    pMqttClientSess->lwtInfo.dup = false;
    pMqttClientSess->lwtInfo.retain = false;

    int returnStatus = EXIT_SUCCESS;

    returnStatus = initializeMqtt(pMqttClientSess);

    if (returnStatus == EXIT_SUCCESS) {
        returnStatus = connectToServerWithBackoffRetries(pMqttClientSess, CONNECTION_RETRY_MAX_ATTEMPTS);
    }

    if (returnStatus == EXIT_SUCCESS) {
        pMqttClientSess->mqttState = MQTT_CONNECTED;

        if (pMqttTaskCtrl->mqtt_keepalive_created == false) {
            pMqttTaskCtrl->mqtt_keepalive_timer = (nt_osal_timer_handle_t)nt_create_timer(
                mqtt_keepalive_timer_cb, NULL, pMqttTaskCtrl->mqttkeepalive_time_bmps, FALSE);

            if (nt_start_timer(pMqttTaskCtrl->mqtt_keepalive_timer) != NT_TIMER_SUCCESS) {
                MQTT_CLIENT_PRINTF("MQTT keepalive timer start failed\n");
            }
        }
        pMqttTaskCtrl->mqtt_keepalive_created = true;
#ifdef CONFIG_QAT_MQTT_DEMO
        snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_CONNECTED:%d,%d,\"%s\",%d,%d",
                 pMqttClientSess->sessionIndex, pMqttClientSess->mqttTransportScheme,
                 pMqttClientSess->serverInfo.pHostName, pMqttClientSess->serverInfo.port, 1);
        QAT_Response_Str(QAT_RC_QUIET, buffer);
#else
        MQTT_CLIENT_PRINTF("MQTT connect successfully.\n");
#endif
    } else {
        pMqttClientSess->mqttState = MQTT_DISCONNECT;
    }

    if (mqttThreadCreated == false) {
        if (nt_qurt_thread_create(mqttc_task, "mqtt_client_task", 4096, pMqttClientSess, 6, NULL) != pdPASS) {
            MQTT_CLIENT_PRINTF("MQTT main thread create fail\n");
        } else {
            mqttThreadCreated = true;
        }
    }

    if (mqttRxThreadCreated == false) {
        if (nt_qurt_thread_create(mqttc_rx_task, "mqtt_rx_client_task", 4096, pMqttClientSess, 6, NULL) != pdPASS) {
            MQTT_CLIENT_PRINTF("MQTT rx thread create fail\n");
        } else {
            mqttRxThreadCreated = true;
        }
    }

end:
    return QAPI_OK;
fail:
    cleanupConnectInfo(pMqttClientSess);

    return QAPI_ERR_INVALID_PARAM;
}

qapi_Status_t mqttc_sub_info_query(void)
{
    MQTTClientSession_t *pMqttClientSess;
    uint16_t sessionIndex = 0;
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
    for (sessionIndex = 0; sessionIndex < MQTT_DEMO_SESSION_NUM; sessionIndex++) {
        pMqttClientSess = &mqtt_client_sess[sessionIndex];
#ifdef CONFIG_QAT_MQTT_DEMO
        if (pMqttClientSess->mqttState == MQTT_CONNECTED) {
            for (uint16_t i = 0; i < MQTT_SUB_TOPIC_PER_SESSION_MAX; i++) {
                if (pMqttClientSess->subscribeInfo[i].topicFilterLength != 0) {
                    snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+MQTTSUB:%d,%d,\"%s\",%d\n", sessionIndex,
                             pMqttClientSess->mqttState, pMqttClientSess->subscribeInfo[i].pTopicFilter,
                             pMqttClientSess->subscribeInfo[i].qos);
                    QAT_Response_Str(QAT_RC_QUIET, buffer);
                }
            }
        }
#else

#endif
    }
}

qapi_Status_t mqttc_subscribe(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t index = 1;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    MQTTTaskCtrl_t *pMqttTaskCtrl = &mqtt_task_ctrl;

    if (Parameter_Count < 2) {
        mqtt_client_help();
        return QAPI_ERR_INVALID_PARAM;
    }

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
    index = 0;
#endif

    /*            [0]   [1]
        mqttc subscribe <session_id> -t <topic filter> [-q <requested_qos>]
    */
    /*
        +MQTTSUB=<session_id>,<\"topic\">,<requested_qos>
    */
    sessionIndex = Parameter_List[index].Integer_Value;
    isIntegerValid = Parameter_List[index].Integer_Is_Valid;

    if (isIntegerValid == false) {
        MQTT_CLIENT_PRINTF("Invalid session id %s\n", Parameter_List[index].String_Value);
        goto end;
    }

    if (sessionIndex > MQTT_DEMO_SESSION_NUM) {
        MQTT_CLIENT_PRINTF("Invalid session id %d\n", sessionIndex);
        goto end;
    }

    index++;
    MQTTClientSession_t *pMqttClientSess = &mqtt_client_sess[sessionIndex];

    if (pMqttClientSess->mqttState != MQTT_CONNECTED) {
        MQTT_CLIENT_PRINTF("MQTT state:%d, is not in connected mode\n", pMqttClientSess->mqttState);
        goto end;
    }

    MQTTClientCMD_t *pMqttCommand = &mqtt_client_cmd[sessionIndex];

    while (index < Parameter_Count) {
#ifdef CONFIG_QAT_MQTT_DEMO
        //  <topic>
        if (index == 1)
#else
        if (0 == strcmp(Parameter_List[index].String_Value, "-t"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif
            pMqttCommand->mqtt_cmd.subscribe.topicFilterLength = strlen(Parameter_List[index].String_Value);
            pMqttCommand->mqtt_cmd.subscribe.pTopicFilter = malloc(strlen(Parameter_List[index].String_Value) + 1);

            if (pMqttCommand->mqtt_cmd.subscribe.pTopicFilter == NULL) {
                MQTT_CLIENT_PRINTF("MQTT session:%d malloc topic filter fail.\n", sessionIndex);
                goto fail;
            }

            memcpy((char *)(pMqttCommand->mqtt_cmd.subscribe.pTopicFilter), Parameter_List[index].String_Value,
                   strlen(Parameter_List[index].String_Value) + 1);

            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        //  <requested_qos>
        else if (index == 2)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "-q"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif

            if (Parameter_List[index].Integer_Value < MQTTQoS0 || Parameter_List[index].Integer_Value > MQTTQoS2) {
                MQTT_CLIENT_PRINTF("MQTT session:%d parameter qos error.\n", sessionIndex);
                goto fail;
            }

            pMqttCommand->mqtt_cmd.subscribe.qos = Parameter_List[index].Integer_Value;

            index++;
        } else {
            MQTT_CLIENT_PRINTF("Invalid parameter %s\n", Parameter_List[index].String_Value);
            index++;
            goto fail;
        }
    }

    if (pMqttCommand->mqtt_cmd.subscribe.pTopicFilter == NULL) {
        MQTT_CLIENT_PRINTF("Error:topic filter is not configured\n\n");
        goto fail;
    }

    pMqttCommand->cmd_type = MQTT_CMD_SUB;

    qurt_signal_set(&pMqttTaskCtrl->mqtt_client_signal, MQTT_SUB);

end:
    return QAPI_OK;

fail:

    if (pMqttCommand->mqtt_cmd.subscribe.pTopicFilter) {
        free((char *)pMqttCommand->mqtt_cmd.subscribe.pTopicFilter);
        pMqttCommand->mqtt_cmd.subscribe.pTopicFilter = NULL;
    }

    return QAPI_ERR_INVALID_PARAM;
}

qapi_Status_t mqttc_unsubscribe(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t index = 1;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    MQTTTaskCtrl_t *pMqttTaskCtrl = &mqtt_task_ctrl;

    if (Parameter_Count < 2) {
        mqtt_client_help();
        return QAPI_ERR_INVALID_PARAM;
    }

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
    index = 0;
#endif
    /*          [0]   [1]
     mqttc unsubscribe <session_id> -t <topic filter>
    */
    /*
     +MQTTSUB=<session_id>,<\"topic\">
    */

    sessionIndex = Parameter_List[index].Integer_Value;
    isIntegerValid = Parameter_List[index].Integer_Is_Valid;

    if (isIntegerValid == false) {
        MQTT_CLIENT_PRINTF("Invalid session id %s\n", Parameter_List[index].String_Value);
        goto end;
    }

    if (sessionIndex > MQTT_DEMO_SESSION_NUM) {
        MQTT_CLIENT_PRINTF("Invalid session id %d\n", sessionIndex);
        goto end;
    }

    index++;
    MQTTClientSession_t *pMqttClientSess = &mqtt_client_sess[sessionIndex];

    if (pMqttClientSess->mqttState != MQTT_CONNECTED) {
        MQTT_CLIENT_PRINTF("MQTT state:%d, is not in connected mode\n", pMqttClientSess->mqttState);
        goto end;
    }

    MQTTClientCMD_t *pMqttCommand = &mqtt_client_cmd[sessionIndex];

    while (index < Parameter_Count) {
#ifdef CONFIG_QAT_MQTT_DEMO
        //  <topic>
        if (index == 1)
#else
        if (0 == strcmp(Parameter_List[index].String_Value, "-t"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif
            pMqttCommand->mqtt_cmd.unsubscribe.topicFilterLength = strlen(Parameter_List[index].String_Value);
            pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter = malloc(strlen(Parameter_List[index].String_Value) + 1);

            if (pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter == NULL) {
                MQTT_CLIENT_PRINTF("MQTT session:%d alloc topic filter fail.\n", sessionIndex);
                goto fail;
            }

            memcpy((char *)(pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter), Parameter_List[index].String_Value,
                   strlen(Parameter_List[index].String_Value) + 1);

            index++;
        } else {
            MQTT_CLIENT_PRINTF("Invalid parameter %s\n", Parameter_List[index].String_Value);
            index++;
            goto fail;
        }
    }

    if (pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter == NULL) {
        MQTT_CLIENT_PRINTF("Error:topic filter is not configured\n\n");
        goto fail;
    }

    pMqttCommand->cmd_type = MQTT_CMD_UNSUB;

    qurt_signal_set(&pMqttTaskCtrl->mqtt_client_signal, MQTT_UNSUB);

end:
    return QAPI_OK;
fail:

    if (pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter) {
        free((char *)pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter);
        pMqttCommand->mqtt_cmd.unsubscribe.pTopicFilter = NULL;
    }

    return QAPI_ERR_INVALID_PARAM;
}

#ifdef CONFIG_QAT_MQTT_DEMO
qapi_Status_t mqttc_publishRaw_Block(uint32_t len, char *block_buf)
{
    MQTTClientSession_t *pMqttClientSess = &mqtt_client_sess[sessionIndex_raw];
    MQTTTaskCtrl_t *pMqttTaskCtrl = &mqtt_task_ctrl;

    uint32_t signal = 0;

    if (pMqttClientSess->mqttState != MQTT_CONNECTED) {
        MQTT_CLIENT_PRINTF("MQTT state:%d, is not in connected mode\n", pMqttClientSess->mqttState);
        goto end;
    }

    MQTTClientCMD_t *pMqttCommand = &mqtt_client_cmd[sessionIndex_raw];

    if (pMqttCommand->cmd_type != MQTT_CMD_NONE) {
        MQTT_CLIENT_PRINTF("MQTT session:%d command is busy\n", sessionIndex_raw);
        goto end;
    }

    pMqttCommand->mqtt_cmd.publish.payloadLength = len;
    pMqttCommand->mqtt_cmd.publish.pPayload = malloc(len);

    if (pMqttCommand->mqtt_cmd.publish.pPayload == NULL) {
        MQTT_CLIENT_PRINTF("MQTT session:%d malloc message fail.\n", sessionIndex_raw);
        goto fail;
    }

    memcpy((char *)pMqttCommand->mqtt_cmd.publish.pPayload, block_buf, len);

    if (pMqttCommand->mqtt_cmd.publish.pTopicName == NULL) {
        MQTT_CLIENT_PRINTF("Error:topic is not configured\n");
        goto fail;
    }

    pMqttCommand->cmd_type = MQTT_CMD_PUB;

    qurt_signal_set(&pMqttTaskCtrl->mqtt_client_signal, MQTT_PUB_RAW_START);

    signal = qurt_signal_wait(&pMqttTaskCtrl->mqtt_client_signal, MQTT_PUB_RAW_DONE, QURT_SIGNAL_ATTR_CLEAR_MASK);

end:
    return QAPI_OK;

fail:

    if (pMqttCommand->mqtt_cmd.publish.pTopicName) {
        free((char *)pMqttCommand->mqtt_cmd.publish.pTopicName);
        pMqttCommand->mqtt_cmd.publish.pTopicName = NULL;
    }

    if (pMqttCommand->mqtt_cmd.publish.pPayload) {
        free((char *)pMqttCommand->mqtt_cmd.publish.pPayload);
        pMqttCommand->mqtt_cmd.publish.pPayload = NULL;
    }

    return QAPI_ERR_INVALID_PARAM;
}

qapi_Status_t mqttc_publishRaw_Cache(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t index = 1;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;

    if (Parameter_Count < 2) {
        mqtt_client_help();
        return QAPI_ERR_INVALID_PARAM;
    }

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
    index = 0;
#endif

    /*                [0]   [1]
     +MQTTPUBRAW=<session_id>,<\"topic\">,<length>,<qos_level>,<retain>
   */

    sessionIndex = Parameter_List[index].Integer_Value;
    isIntegerValid = Parameter_List[index].Integer_Is_Valid;

    sessionIndex_raw = sessionIndex;

    if (isIntegerValid == false) {
        MQTT_CLIENT_PRINTF("Invalid session id %s\n", Parameter_List[index].String_Value);
        goto end;
    }

    if (sessionIndex > MQTT_DEMO_SESSION_NUM) {
        MQTT_CLIENT_PRINTF("Invalid session id %d\n", sessionIndex);
        goto end;
    }

    index++;
    MQTTClientSession_t *pMqttClientSess = &mqtt_client_sess[sessionIndex];

    if (pMqttClientSess->mqttState != MQTT_CONNECTED) {
        MQTT_CLIENT_PRINTF("MQTT state:%d, is not in connected mode\n", pMqttClientSess->mqttState);
        goto end;
    }

    MQTTClientCMD_t *pMqttCommand = &mqtt_client_cmd[sessionIndex];

    if (pMqttCommand->cmd_type != MQTT_CMD_NONE) {
        MQTT_CLIENT_PRINTF("MQTT session:%d command is busy\n", sessionIndex);
        goto end;
    }

    // assert(pMqttCommand->mqtt_cmd.publish.pTopicName == NULL);
    if (pMqttCommand->mqtt_cmd.publish.pTopicName != NULL) {
        free(pMqttCommand->mqtt_cmd.publish.pTopicName);
        pMqttCommand->mqtt_cmd.publish.pTopicName = NULL;
    }

    /* Some fields not used by this demo so start with everything at 0. */
    (void)memset((void *)&(pMqttCommand->mqtt_cmd.publish), 0x00, sizeof(MQTTPublishInfo_t));

    while (index < Parameter_Count) {
#ifdef CONFIG_QAT_MQTT_DEMO
        //  <\"topic\">
        if (index == 1)
#else
        if (0 == strcmp(Parameter_List[index].String_Value, "-t"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif
            pMqttCommand->mqtt_cmd.publish.topicNameLength = strlen(Parameter_List[index].String_Value);
            pMqttCommand->mqtt_cmd.publish.pTopicName = malloc(strlen(Parameter_List[index].String_Value) + 1);

            if (pMqttCommand->mqtt_cmd.publish.pTopicName == NULL) {
                MQTT_CLIENT_PRINTF("MQTT session:%d alloc topic name fail.\n", sessionIndex);
                goto fail;
            }

            memcpy((char *)(pMqttCommand->mqtt_cmd.publish.pTopicName), Parameter_List[index].String_Value,
                   strlen(Parameter_List[index].String_Value) + 1);
            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        //  <qos_level>
        else if (index == 3)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "-q"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif

            if (Parameter_List[index].Integer_Value < MQTTQoS0 || Parameter_List[index].Integer_Value > MQTTQoS2) {
                MQTT_CLIENT_PRINTF("MQTT session:%d qos out of range.\n", sessionIndex);
                goto fail;
            }

            pMqttCommand->mqtt_cmd.publish.qos = Parameter_List[index].Integer_Value;

            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        //  <length>
        else if (index == 2)
#endif
        {
            total_len_one_raw = Parameter_List[index].Integer_Value;
            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        //  <retain>
        else if (index == 4)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "-r"))
#endif
        {
            index++;
            pMqttCommand->mqtt_cmd.publish.retain = true;
        } else {
            MQTT_CLIENT_PRINTF("Invalid parameter %s\n", Parameter_List[index].String_Value);
            index++;
            goto fail;
        }
    }

    if (pMqttCommand->mqtt_cmd.publish.pTopicName == NULL) {
        MQTT_CLIENT_PRINTF("Error:topic is not configured\n");
        goto fail;
    }

    // pMqttCommand->cmd_type = MQTT_CMD_PUB;

end:
    return QAPI_OK;

fail:

    if (pMqttCommand->mqtt_cmd.publish.pTopicName) {
        free((char *)pMqttCommand->mqtt_cmd.publish.pTopicName);
        pMqttCommand->mqtt_cmd.publish.pTopicName = NULL;
    }

    if (pMqttCommand->mqtt_cmd.publish.pPayload) {
        free((char *)pMqttCommand->mqtt_cmd.publish.pPayload);
        pMqttCommand->mqtt_cmd.publish.pPayload = NULL;
    }

    return QAPI_ERR_INVALID_PARAM;
}
#endif

qapi_Status_t mqttc_publish(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t index = 1;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    uint32_t signal = 0;
    MQTTTaskCtrl_t *pMqttTaskCtrl = &mqtt_task_ctrl;

    if (Parameter_Count < 2) {
        mqtt_client_help();
        return QAPI_ERR_INVALID_PARAM;
    }

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
    index = 0;
#endif

    /*      [0]   [1]
    mqttc publish <session_id> -t <topic> [-q <qos_level>] -m <message> [-r]
     */
    /*                [0]   [1]
     +MQTTPUB=<session_id>,<\"topic\">,<qos_level>,<\"message\">,<retain>
   */

    sessionIndex = Parameter_List[index].Integer_Value;
    isIntegerValid = Parameter_List[index].Integer_Is_Valid;

    if (isIntegerValid == false) {
        MQTT_CLIENT_PRINTF("Invalid session id %s\n", Parameter_List[index].String_Value);
        goto end;
    }

    if (sessionIndex > MQTT_DEMO_SESSION_NUM) {
        MQTT_CLIENT_PRINTF("Invalid session id %d\n", sessionIndex);
        goto end;
    }

    index++;
    MQTTClientSession_t *pMqttClientSess = &mqtt_client_sess[sessionIndex];

    if (pMqttClientSess->mqttState != MQTT_CONNECTED) {
        MQTT_CLIENT_PRINTF("MQTT state:%d, is not in connected mode\n", pMqttClientSess->mqttState);
        goto end;
    }

    MQTTClientCMD_t *pMqttCommand = &mqtt_client_cmd[sessionIndex];

    if (pMqttCommand->cmd_type != MQTT_CMD_NONE) {
        MQTT_CLIENT_PRINTF("MQTT session:%d command is busy\n", sessionIndex);
        goto end;
    }

    assert(pMqttCommand->mqtt_cmd.publish.pTopicName == NULL);

    /* Some fields not used by this demo so start with everything at 0. */
    (void)memset((void *)&(pMqttCommand->mqtt_cmd.publish), 0x00, sizeof(MQTTPublishInfo_t));

    while (index < Parameter_Count) {
#ifdef CONFIG_QAT_MQTT_DEMO
        //  <\"topic\">
        if (index == 1)
#else
        if (0 == strcmp(Parameter_List[index].String_Value, "-t"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif
            pMqttCommand->mqtt_cmd.publish.topicNameLength = strlen(Parameter_List[index].String_Value);
            pMqttCommand->mqtt_cmd.publish.pTopicName = malloc(strlen(Parameter_List[index].String_Value) + 1);

            if (pMqttCommand->mqtt_cmd.publish.pTopicName == NULL) {
                MQTT_CLIENT_PRINTF("MQTT session:%d alloc topic name fail.\n", sessionIndex);
                goto fail;
            }

            memcpy((char *)(pMqttCommand->mqtt_cmd.publish.pTopicName), Parameter_List[index].String_Value,
                   strlen(Parameter_List[index].String_Value) + 1);
            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        //  <qos_level>
        else if (index == 2)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "-q"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif

            if (Parameter_List[index].Integer_Value < MQTTQoS0 || Parameter_List[index].Integer_Value > MQTTQoS2) {
                MQTT_CLIENT_PRINTF("MQTT session:%d qos out of range.\n", sessionIndex);
                goto fail;
            }

            pMqttCommand->mqtt_cmd.publish.qos = Parameter_List[index].Integer_Value;

            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        //  <\"message\">
        else if (index == 3)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "-m"))
#endif
        {
#ifndef CONFIG_QAT_MQTT_DEMO
            index++;
#endif
            pMqttCommand->mqtt_cmd.publish.payloadLength = strlen(Parameter_List[index].String_Value);
            total_len_one_raw = pMqttCommand->mqtt_cmd.publish.payloadLength;
            pMqttCommand->mqtt_cmd.publish.pPayload = malloc(pMqttCommand->mqtt_cmd.publish.payloadLength + 1);

            if (pMqttCommand->mqtt_cmd.publish.pPayload == NULL) {
                MQTT_CLIENT_PRINTF("MQTT session:%d malloc message fail.\n", sessionIndex);
                goto fail;
            }

            memcpy((char *)pMqttCommand->mqtt_cmd.publish.pPayload, Parameter_List[index].String_Value,
                   pMqttCommand->mqtt_cmd.publish.payloadLength + 1);
            index++;
        }
#ifdef CONFIG_QAT_MQTT_DEMO
        //  <retain>
        else if (index == 4)
#else
        else if (0 == strcmp(Parameter_List[index].String_Value, "-r"))
#endif
        {

#ifdef CONFIG_QAT_MQTT_DEMO
            pMqttCommand->mqtt_cmd.publish.retain = Parameter_List[index].Integer_Value != 0 ? true : false;
#else
            pMqttCommand->mqtt_cmd.publish.retain = true;
#endif
            index++;
        } else {
            MQTT_CLIENT_PRINTF("Invalid parameter %s\n", Parameter_List[index].String_Value);
            index++;
            goto fail;
        }
    }

    if (pMqttCommand->mqtt_cmd.publish.pTopicName == NULL) {
        MQTT_CLIENT_PRINTF("Error:topic is not configured\n");
        goto fail;
    }

    pMqttCommand->cmd_type = MQTT_CMD_PUB;

    qurt_signal_set(&pMqttTaskCtrl->mqtt_client_signal, MQTT_PUB_RAW_START);

    signal = qurt_signal_wait(&pMqttTaskCtrl->mqtt_client_signal, MQTT_PUB_RAW_DONE, QURT_SIGNAL_ATTR_CLEAR_MASK);
end:
    return QAPI_OK;

fail:

    if (pMqttCommand->mqtt_cmd.publish.pTopicName) {
        free((char *)pMqttCommand->mqtt_cmd.publish.pTopicName);
        pMqttCommand->mqtt_cmd.publish.pTopicName = NULL;
    }

    if (pMqttCommand->mqtt_cmd.publish.pPayload) {
        free((char *)pMqttCommand->mqtt_cmd.publish.pPayload);
        pMqttCommand->mqtt_cmd.publish.pPayload = NULL;
    }

    return QAPI_ERR_INVALID_PARAM;
}

qapi_Status_t mqttc_disconnect(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t index = 1;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    MQTTTaskCtrl_t *pMqttTaskCtrl = &mqtt_task_ctrl;

#ifdef CONFIG_QAT_MQTT_DEMO
    if (Parameter_Count < 1)
#else
    if (Parameter_Count < 2)
#endif
    {
        mqtt_client_help();
        return QAPI_ERR_INVALID_PARAM;
    }

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
    index = 0;
#endif

    /*         [0]   [1]
     mqttc disconnect <session_id>
    */
    /*
    +MQTTDISCONN=<session_id>
    */

    sessionIndex = Parameter_List[index].Integer_Value;
    isIntegerValid = Parameter_List[index].Integer_Is_Valid;

    if (isIntegerValid == false) {
        MQTT_CLIENT_PRINTF("Invalid session id %s\n", Parameter_List[index].String_Value);
        goto fail;
    }

    if (sessionIndex > MQTT_DEMO_SESSION_NUM) {
        MQTT_CLIENT_PRINTF("Invalid session id %d\n", sessionIndex);
        goto fail;
    }

    index++;
    MQTTClientCMD_t *pMqttCommand = &mqtt_client_cmd[sessionIndex];
    MQTTClientSession_t *pMqttClientSess = &mqtt_client_sess[sessionIndex];
    if (pMqttClientSess->mqttState == MQTT_FORCE_DISCONNECT) {
        MQTT_CLIENT_PRINTF("MQTT state:%d, already disconnected\n", pMqttClientSess->mqttState);
        goto fail;
    }
    pMqttCommand->cmd_type = MQTT_CMD_DISC;

    qurt_signal_set(&pMqttTaskCtrl->mqtt_client_signal, MQTT_DISCONN);

    return QAPI_OK;
fail:
    return QAPI_ERR_INVALID_PARAM;
}
void cleanupMqttSession(MQTTClientSession_t *pMqttClientSess)
{
    assert(pMqttClientSess != NULL);

    cleanupNetworkCredentials(pMqttClientSess);
    cleanupConnectInfo(pMqttClientSess);
}
qapi_Status_t mqttc_destroy(uint32_t sessionIndex)
{
    if (sessionIndex > MQTT_DEMO_SESSION_NUM) {
        MQTT_CLIENT_PRINTF("Invalid session id %d\n", sessionIndex);
        return QAPI_ERR_INVALID_PARAM;
    }

#ifdef CONFIG_QAT_MQTT_DEMO
    char buffer[WRTMEM_STR_BUFFER_LENGTH];
#endif

    /*         [0]   [1]
     mqttc destroy <session_id>
    */
    /*
    +MQTTDESTROY=<session_id>
    */
    MQTTClientSession_t *pMqttClientSess = &mqtt_client_sess[sessionIndex];

    if ((pMqttClientSess->mqttState != MQTT_FORCE_DISCONNECT) && (pMqttClientSess->mqttState != MQTT_INIT)) {
        MQTT_CLIENT_PRINTF("MQTT state:%d, execute disconnect command.\n", pMqttClientSess->mqttState);
        return QAPI_ERR_INVALID_PARAM;
    }

    cleanupMqttSession(pMqttClientSess);
    pMqttClientSess->mqttState = MQTT_INIT;
    /* Log message indicating destroy successfully. */
#ifdef CONFIG_QAT_MQTT_DEMO
    snprintf(buffer, WRTMEM_STR_BUFFER_LENGTH, "+EVT:MQTT_DESTROYED:%d\n", sessionIndex);
    QAT_Response_Str(QAT_RC_QUIET, buffer);
#else
    MQTT_CLIENT_PRINTF("MQTT session:%d destroy successfully.\n", pMqttClientSess->sessionIndex);
#endif

    return QAPI_OK;
}

qapi_Status_t mqttc_demo(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    char *cmd;
    uint32_t index = 0;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;

    /* The following commands need <session_id> parameter */
    if (Parameter_Count < 2 || Parameter_List == NULL) {
        mqtt_client_help();
        return QAPI_ERR_INVALID_PARAM;
    }

    sessionIndex = Parameter_List[1].Integer_Value;
    isIntegerValid = Parameter_List[1].Integer_Is_Valid;

    if (isIntegerValid == false) {
        MQTT_CLIENT_PRINTF("Invalid session id %s\n", Parameter_List[1].String_Value);
        goto end;
    }

    if (sessionIndex >= MQTT_DEMO_SESSION_NUM) {
        MQTT_CLIENT_PRINTF("Invalid session id %d\n", sessionIndex);
        goto end;
    }

    index++;
    cmd = Parameter_List[0].String_Value;

    /*    [0]   [1]
     mqttc init <session_id> [-i client_id] [--ca file] [--cert file] [--key file] [--sni <sni_name>] [--alpn
     <protocol_name>]
    */

    if (strcmp(cmd, "init") == 0) {
        mqttc_init(Parameter_Count, Parameter_List);
    } else if (strcmp(cmd, "connect") == 0) {
        mqttc_connect(Parameter_Count, Parameter_List);
    } else if (strcmp(cmd, "subscribe") == 0) {
        mqttc_subscribe(Parameter_Count, Parameter_List);
    } else if (strcmp(cmd, "unsubscribe") == 0) {
        mqttc_unsubscribe(Parameter_Count, Parameter_List);
    } else if (strcmp(cmd, "publish") == 0) {
        mqttc_publish(Parameter_Count, Parameter_List);
    } else if (strcmp(cmd, "disconnect") == 0) {
        mqttc_disconnect(Parameter_Count, Parameter_List);
    } else if (strcmp(cmd, "destroy") == 0) {
        mqttc_destroy(sessionIndex);
    } else if (strcmp(cmd, "--help") == 0) {
        mqtt_client_help();
    } else {
        MQTT_CLIENT_PRINTF("Invalid command %s\n", cmd);
        mqtt_client_help();
    }

end:
    return QAPI_ERR_INVALID_PARAM;
}
