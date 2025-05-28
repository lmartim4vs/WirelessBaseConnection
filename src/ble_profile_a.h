/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef BLE_PROFILE_A_H
#define BLE_PROFILE_A_H

#include "esp_gatts_api.h"

/**
 * @brief Profile A event handler
 * 
 * @param event GATTS event type  
 * @param gatts_if GATT server interface
 * @param param Event parameters
 */
void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#endif // BLE_PROFILE_A_H