/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef BLE_PROFILE_B_H
#define BLE_PROFILE_B_H

#include "esp_gatts_api.h"

/**
 * @brief Profile B event handler
 * 
 * @param event GATTS event type
 * @param gatts_if GATT server interface
 * @param param Event parameters
 */
void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/**
 * @brief Initialize Profile B in the global profile table
 */
void ble_profile_b_init(void);

#endif // BLE_PROFILE_B_H