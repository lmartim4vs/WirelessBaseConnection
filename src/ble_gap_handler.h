/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef BLE_GAP_HANDLER_H
#define BLE_GAP_HANDLER_H

#include "esp_gap_ble_api.h"
#include "esp_err.h"

/**
 * @brief Initialize BLE GAP functionality
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ble_gap_init(void);

/**
 * @brief Start BLE advertising
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ble_gap_start_advertising(void);

/**
 * @brief Set device name from example configuration
 */
void ble_gap_set_device_name_from_example(void);

/**
 * @brief GAP event handler callback
 * 
 * @param event GAP event type
 * @param param Event parameters
 */
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

#endif // BLE_GAP_HANDLER_H