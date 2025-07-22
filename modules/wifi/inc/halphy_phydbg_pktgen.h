/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HALPHY_PHYDBG_PKTGEN_H
#define _HALPHY_PHYDBG_PKTGEN_H

#include <stdint.h>
#include <stdbool.h>
#include "fermion_reg.h"
#include "phyCalUtils.h"

#define DEFAULT_PAYLOAD_LEN       1500  // Packet Size in Bytes
#define DEFAULT_INTER_FRAME_SPACE 1390
#define DEFAULT_TX_DEMANDED_POWER 10

typedef enum mpi_packet_type_e {
    mpi_packet_type_11a = 0,         // 000 = 11a
    mpi_packet_type_11b = 1,         // 001 = 11b
    mpi_packet_type_greenfield = 2,  // 010 = Greenfield (HT) 802.11n only
    mpi_packet_type_mixed = 3,       // 011 = Mixed - mode 802.11 b/g/n
    mpi_packet_type_11ac = 4,        // 100 = 802.11ac (VHT)
    mpi_packet_type_11ba = 5,        // 101 = 11ba
} mpi_packet_type_t;

typedef enum mpi_bandwidth_mode_e {
    mpi_bandwidth_mode_20 = 0,  // 00 = 20MHz
    mpi_bandwidth_mode_40 = 1,  // 01 = 40MHz
    mpi_bandwidth_mode_80 = 2,  // 10 = 80MHz
} mpi_bandwidth_mode_t;

typedef enum mpi_11b_rate_e {
    mpi_11b_rate_invalid = 0,   // Don't care
    mpi_11b_rate_1_mbps = 0,    // 2'b00 = 1 Mbps
    mpi_11b_rate_2_mbps = 1,    // 2'b01 = 2 Mbps
    mpi_11b_rate_5_5_mbps = 2,  // 2'b10 = 5.5 Mbps
    mpi_11b_rate_11_mbps = 3,   // 2'b11 = 11 Mbps
} mpi_11b_rate_t;

typedef enum mpi_11b_mode_e {
    mpi_11b_mode_preamble_invalid = 0,
    mpi_11b_mode_preamble_short = 0,  // 1'b0 = 11b short preamble
    mpi_11b_mode_preamble_long = 1,   // 1'b1 = 11b long preamble
} mpi_11b_mode_t;

/*
 PSDU Rate
 11a/11n rate index
 If packet_type is 11a, field encoded as
 4'b1011 = 6Mbps
 4'b1111 = 9Mbps
 4'b1010 = 12Mbps
 4'b1110 = 18Mbps
 4'b1001 = 24Mbps
 4'b1101 = 36Mbps
 4'b1000 = 48Mbps
 4'b1100 = 54Mbps
 If packet_type is 11n/11ac, field represents MCS value
 */
typedef enum mpi_rate_legacy_e {  // 802.11 a/g rates
    mpi_rate_11a_6_mbps = 0b1011,
    mpi_rate_11a_9_mbps = 0b1111,
    mpi_rate_11a_12_mbps = 0b1010,
    mpi_rate_11a_18_mbps = 0b1110,
    mpi_rate_11a_24_mbps = 0b1001,
    mpi_rate_11a_36_mbps = 0b1101,
    mpi_rate_11a_48_mbps = 0b1000,
    mpi_rate_11a_54_mbps = 0b1100,
} mpi_rate_legacy_t;

typedef enum mpi_rate_ht_e {  // 802.11 n rates
    mpi_rate_ht_0 = 0,
    mpi_rate_ht_1 = 1,
    mpi_rate_ht_2 = 2,
    mpi_rate_ht_3 = 3,
    mpi_rate_ht_4 = 4,
    mpi_rate_ht_5 = 5,
    mpi_rate_ht_6 = 6,
    mpi_rate_ht_7 = 7,
    mpi_rate_ht_max = 8,
} mpi_rate_ht_t;

// 802.11ba rates
typedef enum mpi_rate_11ba_e {
    mpi_rate_11ba_low = 0,   // 0 = low rate (0.0625 Mbps)
    mpi_rate_11ba_high = 1,  // 1 = high rate (0.25 Mbps).
} mpi_rate_11ba_t;

typedef enum mpi_guard_interval_e {
    mpi_guard_interval_invalid = 0,
    mpi_guard_interval_normal = 0,  // 0 = 800ns normal guard interval
    mpi_guard_interval_short = 1,   // 1 = 400ns short guard interval
} mpi_guard_interval_t;

// TLV2 Gen 2 PARM_GI
typedef enum guard_interval_e { gi_0 = 0, gi_400 = 1, gi_800 = 2, gi_max = 3 } guard_interval_t;

typedef enum mpi_fec_mode_e {
    mpi_fec_mode_bcc = 0,   // BCC Mode
    mpi_fec_mode_ldpc = 1,  // LDPC Mode
} mpi_fec_mode_t;

// TLV2 Gen 2 rate index PARM_RATEBITINDEX
typedef enum mpi_rate_e {
    rate_1mbps_l = 0,
    rate_2mbps_l = 1,
    rate_5_5mbps_l = 2,
    rate_11mbps_l = 3,
    rate_2mbps_s = 4,
    rate_5_5mbps_s = 5,
    rate_11mbps_s = 6,
    rate_6mbps = 10,
    rate_9mbps = 11,
    rate_12mbps = 12,
    rate_18mbps = 13,
    rate_24mbps = 14,
    rate_36mbps = 15,
    rate_48mbps = 16,
    rate_54mbps = 17,
    rate_mcs_0_20 = 20,
    rate_mcs_1_20 = 21,
    rate_mcs_2_20 = 22,
    rate_mcs_3_20 = 23,
    rate_mcs_4_20 = 24,
    rate_mcs_5_20 = 25,
    mpi_rate_max = 26
} mpi_rate_t;

typedef struct mpi_rate_settings_s {
    mpi_packet_type_t mpi_packet_type;
    mpi_11b_rate_t mpi_11b_rate;
    mpi_11b_mode_t mpi_11b_mode;
    uint8_t rate_index;
} mpi_rate_settings_t;

typedef struct phydbg_playback_packet_header_s {  // INFO0 and INFO1 words
    uint32_t fcs_flag : 1;  // Frame CheckSum - When set, causes a CRC checksum to be appended to the end of the packet
    uint32_t mpi_cmd_len : 6;         // MPI Command Length (C) - Number of command bytes for this packet
    uint32_t inter_frame_space : 24;  // Time, in clock cycles (12.5 ns), to wait after transmitting this packet before
                                      // sending the next packet (when CC=0)
    uint32_t concatenation_packet_flag : 1;  // Concatenation Packet (CC) When set, indicates that the next packet is
                                             // concatenated on the back of this packet without any inter-frame spacing
    uint32_t payload_len_random : 17;        // Random Payload Length (R) Number of random payload bytes appended to the
                                             // packet payload
    uint32_t payload_len_fixed : 15;  //	Fixed Payload Length (F) Number of fixed payload bytes(stored in memory).
} phydbg_playback_packet_header_t;

typedef struct phydbg_playback_packet_command_bytes_s {
    // byte 0
    uint32_t
        command_length : 8;  // Defines the number of bytes in the command word. 10 bytes for 11n, 13 bytes for 11ac

    // byte 1
    uint32_t packet_type : 3;
    uint32_t bandwidth_mode : 2;
    uint32_t duplicate_flag : 1;  // Duplicate 20MHz subbands in 40MHz or 80MHz RF bandwidth
    // This field is not valid for 802.11b packets and should therefore be set to 1'b0.
    // For duplicate packets, the PHY TX will generate as many 20MHz duplicates as necessary to fill the active band.As
    // an example, setting the bandwidth mode field to 2'b01 will result in the TXFIR producing 2 duplicate 20MHz
    // subbands.
    uint32_t b_rate : 2;  // 11b_rate

    // byte 2
    uint32_t b_mode : 1;         // 11b short preamble flag
    uint32_t rate_index : 4;     // PSDU Rate
    uint32_t ba_rate : 1;        // 11ba rate
    uint32_t rtt_start_ack : 1;  // Start of RTT ACK frame (t3)
    uint32_t rtt_start_m : 1;    // Start of RTT M frame (t4)

    // byte 3
    uint32_t tx_demanded_power : 5;  // 5-bit power level used to look up the gain settings for the RF amplifiers and
                                     // the TXFIR digital gain adjustment
    uint32_t low_power_en : 1;       // Used to select low bias power setting
    uint32_t reserved : 2;           // Reserved

    // byte 4
    uint32_t service_field_override : 1;  // Service field override flag for RTS/CTS control packets – when set,
                                          // bit[6:4] of the service field for legacy RTS/CTS will be overridden
    uint32_t is_dynamic : 1;  // Static/dynamic bandwidth flag for overridden service field scrambler seed field
    uint32_t rts_cts_bw : 2;  // Bandwidth setting for overridden service field scrambler seed field
    uint32_t short_guard_interval : 1;
    uint32_t sounding_flag : 1;
    uint32_t fec_mode_flag : 1;
    uint32_t short_gi_ambiguity : 1;

    // byte 5 & 6
    /*
     * For 802.11a and 802.11n packets, this field specifies the number of bytes in current packet.
     * If set to 0, the PHY TX will generate a Null Data Packet.
     * Although this is a 16-bit field, 11a packets have a maximum length of 4095 bytes.
     * For 11b packets, this field defines the duration of the PSDU in microseconds.
     * The ratio of bytes to microseconds is floor[N/8], where N=1,2,5.5,11 depending on the PSDU rate.
     */
    uint32_t packet_length : 16;

    // byte 7
    /*
     * For OFDM rates only, this defines the QAM constellation order and FEC code rate for data symbols.
     * This field corresponds to the least significant (ls) 6 bits of the MCS field in the 802.11n spec.
     * The ls 4 bits of this field correspond to the RATE field for 802.11a packets.
     */
    uint32_t packet_rate : 6;
    uint32_t reserved_1 : 1;
    uint32_t ampdu_flag : 1;  // This bit is to be copied into the HT-SIG field

    // byte 8 & 9
    uint32_t lsig_length : 12;  // Length field for the L-SIG symbol for 11n mixed-mode and 11ac packets only.
    uint32_t lsig_rate : 4;     // Rate indicated in legacy SIGNAL field (for 11n mixed-mode and 11ac packets). Not sent
                                // for 802.11b packets.

    // removed bytes 10 to 15 as 11ac is not supported in NT and Fermion
    // uint8_t reserved_2[6];

} phydbg_playback_packet_command_bytes_t;

uint16_t phydbg_pktgen_stop(void);
void phydbg_pktgen_send(mpi_rate_t rate, guard_interval_t guard_interval, uint16_t n_packets, uint16_t packet_size,
                        uint8_t tx_demanded_power, uint32_t inter_frame_space, uint32_t rf_warmup);

#endif  //_HALPHY_PHYDBG_PKTGEN_H
