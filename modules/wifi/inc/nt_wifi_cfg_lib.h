/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * nt_wifi_cfg_lib.h
 *
 *  Created on: 06-Dec-2022
 */

#ifndef APPS_WIFI_APP_INC_NT_WIFI_CFG_LIB_H_
#define APPS_WIFI_APP_INC_NT_WIFI_CFG_LIB_H_

#include "wifi_app.h"
#include "iot_wifi.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"

/**
 * brief : parses the network details received from the commissioning
 * device and loads details into the network configuration structure
 * @params : pcreception_buffer : message received from commissioning device
 *           pnw_details        : pointer for network configuration structure to load parsed details
 *           comm_cfg			: pointer to wifi app common cfg
 * @return : TRUE if the asked profile is found in the message else FALSE
 */
uint8_t nt_parse_profile_information();

/**
 * @brief read dev_config
 * read dev_config  and store the cfg in wifi_app global structures
 * @param comm_cfg				pointer to wifi app common config
 * @param pnw_conf 				pointer to WiFiNetworkConfiguration
 * @return TRUE/FALSE
 */
uint8_t nt_app_read_dev_config_and_update_wapp();

/**
 * @brief read wifi config file
 * read wifi_config file and store the cfg in wifi_app global structures
 * @param comm_cfg				pointer to wifi app common config
 * @param pnw_conf 				pointer to WiFiNetworkConfiguration
 * @return TRUE/FALSE
 */
uint8_t nt_app_read_wifi_config();

/**
 * @brief save connection profile
 * write connection profile to wifi config / wifi_config_debug file
 * @param wapp_cntx	pointer to wifi app context
 * @return none
 */
int nt_app_save_cp_to_file();

/*
 * To read wifi_config / wifi_config_debug file after saving the connection profile
 */
uint8_t read_updated_cfg();

/**
 * @brief
 *  Read dev_cfg ->
 *  Read wifi_config / wifi_config_debug ->
 *  Save cfg to wifi_config / wifi_config_debug file again
 *  Read stored configs in wifi_config / wifi_config_debug
 */
uint8_t nt_app_update_cfg();

#endif /* APPS_WIFI_APP_INC_NT_WIFI_CFG_LIB_H_ */
