/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

// Service and Characteristic UUIDs
#define GATTS_SERVICE_UUID_TEST_A   0x00FF
#define GATTS_CHAR_UUID_TEST_A      0xFF01
#define GATTS_DESCR_UUID_TEST_A     0x3333
#define GATTS_NUM_HANDLE_TEST_A     4

#define GATTS_SERVICE_UUID_TEST_B   0x00EE
#define GATTS_CHAR_UUID_TEST_B      0xEE01
#define GATTS_DESCR_UUID_TEST_B     0x2222
#define GATTS_NUM_HANDLE_TEST_B     4

// Device configuration
#define TEST_MANUFACTURER_DATA_LEN  17
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40
#define PREPARE_BUF_MAX_SIZE        1024

// Profile configuration
#define PROFILE_NUM         2
#define PROFILE_A_APP_ID    0
#define PROFILE_B_APP_ID    1

// Advertising configuration flags
#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

// Default device name
#define DEFAULT_DEVICE_NAME "ESP_GATTS_DEMO"

// Connection parameters for iOS compatibility
#define CONN_LATENCY    0
#define CONN_MAX_INT    0x20    // max_int = 0x20*1.25ms = 40ms
#define CONN_MIN_INT    0x10    // min_int = 0x10*1.25ms = 20ms
#define CONN_TIMEOUT    400     // timeout = 400*10ms = 4000ms

// Notification/Indication data size
#define NOTIFY_DATA_SIZE    15

#endif // BLE_CONFIG_H