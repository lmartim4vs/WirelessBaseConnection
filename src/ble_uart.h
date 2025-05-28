/*
 * Simple BLE UART Service
 * Provides TX and RX functionality like a serial port
 */

#ifndef BLE_UART_H
#define BLE_UART_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

// Service and Characteristic UUIDs (using standard Nordic UART Service UUIDs)
#define BLE_UART_SERVICE_UUID       0x6E400001  // Nordic UART Service
#define BLE_UART_TX_CHAR_UUID       0x6E400002  // TX Characteristic (ESP32 sends data)
#define BLE_UART_RX_CHAR_UUID       0x6E400003  // RX Characteristic (ESP32 receives data)

// Configuration
#define BLE_UART_MAX_DATA_LEN       240         // Maximum data length per packet
#define BLE_UART_DEVICE_NAME        "ESP32-UART"

/**
 * @brief Callback function type for receiving data
 * 
 * @param data Pointer to received data
 * @param len Length of received data
 */
typedef void (*ble_uart_rx_callback_t)(uint8_t* data, uint16_t len);

/**
 * @brief Initialize BLE UART service
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ble_uart_init(void);

/**
 * @brief Start BLE UART service (begins advertising)
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ble_uart_start(void);

/**
 * @brief Stop BLE UART service
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ble_uart_stop(void);

/**
 * @brief Send data via BLE UART TX channel
 * 
 * @param data Pointer to data to send
 * @param len Length of data (max BLE_UART_MAX_DATA_LEN)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ble_uart_write(uint8_t* data, uint16_t len);

/**
 * @brief Set callback for receiving data on RX channel
 * 
 * @param callback Function to call when data is received
 */
void ble_uart_set_rx_callback(ble_uart_rx_callback_t callback);

/**
 * @brief Check if a client is connected
 * 
 * @return true if connected, false otherwise
 */
bool ble_uart_is_connected(void);

/**
 * @brief Get connection status as string
 * 
 * @return const char* Status string
 */
const char* ble_uart_get_status_string(void);

#endif // BLE_UART_H