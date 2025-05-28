/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef BLE_GATTS_SERVER_H
#define BLE_GATTS_SERVER_H

#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "esp_err.h"

/**
 * @brief GATT profile structure
 */
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

/**
 * @brief Prepare write environment structure
 */
typedef struct {
    uint8_t *prepare_buf;
    int prepare_len;
} prepare_type_env_t;

/**
 * @brief Initialize BLE GATTS server
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ble_gatts_server_init(void);

/**
 * @brief Main GATTS event handler
 * 
 * @param event GATTS event type
 * @param gatts_if GATT server interface
 * @param param Event parameters
 */
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/**
 * @brief Handle write events for prepared writes
 * 
 * @param gatts_if GATT server interface
 * @param prepare_write_env Prepare write environment
 * @param param Event parameters
 */
void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

/**
 * @brief Handle execute write events
 * 
 * @param prepare_write_env Prepare write environment
 * @param param Event parameters
 */
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

#endif // BLE_GATTS_SERVER_H