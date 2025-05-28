/*
 * Example showing different ways to use the BLE UART
 */

#include "ble_uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char* TAG = "USAGE_EXAMPLE";

// 1. Simple data sending
void example_send_data(void)
{
    if (ble_uart_is_connected()) {
        // Send string
        char* message = "Hello BLE World!";
        ble_uart_write((uint8_t*)message, strlen(message));
        
        // Send binary data
        uint8_t sensor_data[] = {0x01, 0x02, 0x03, 0x04, 0xFF};
        ble_uart_write(sensor_data, sizeof(sensor_data));
        
        // Send JSON-like data
        char json[] = "{\"temp\":25.5,\"humidity\":60}";
        ble_uart_write((uint8_t*)json, strlen(json));
    }
}

// 2. Command processing on RX
void on_command_received(uint8_t* data, uint16_t len)
{
    // Convert to string for easier processing
    char command[len + 1];
    memcpy(command, data, len);
    command[len] = '\0';
    
    ESP_LOGI(TAG, "Processing command: '%s'", command);
    
    // Simple command parser
    if (strncmp(command, "LED_ON", 6) == 0) {
        ESP_LOGI(TAG, "💡 Turning LED ON");
        // gpio_set_level(LED_PIN, 1);
        ble_uart_write((uint8_t*)"LED ON", 6);
        
    } else if (strncmp(command, "LED_OFF", 7) == 0) {
        ESP_LOGI(TAG, "💡 Turning LED OFF");
        // gpio_set_level(LED_PIN, 0);
        ble_uart_write((uint8_t*)"LED OFF", 7);
        
    } else if (strncmp(command, "STATUS", 6) == 0) {
        char status[] = "ESP32 is running OK";
        ble_uart_write((uint8_t*)status, strlen(status));
        
    } else if (strncmp(command, "TEMP", 4) == 0) {
        char temp_response[] = "Temperature: 25.3°C";
        ble_uart_write((uint8_t*)temp_response, strlen(temp_response));
        
    } else {
        char error[] = "Unknown command";
        ble_uart_write((uint8_t*)error, strlen(error));
    }
}

// 3. Sensor data streaming
void stream_sensor_data_task(void* arg)
{
    while (1) {
        if (ble_uart_is_connected()) {
            // Simulate sensor readings
            float temperature = 25.0 + (rand() % 100) / 10.0f;
            float humidity = 50.0 + (rand() % 500) / 10.0f;
            
            // Send as formatted string
            char sensor_str[64];
            snprintf(sensor_str, sizeof(sensor_str), 
                    "T:%.1f,H:%.1f", temperature, humidity);
            ble_uart_write((uint8_t*)sensor_str, strlen(sensor_str));
            
            // Or send as binary struct
            typedef struct {
                float temp;
                float hum;
                uint32_t timestamp;
            } sensor_data_t;
            
            sensor_data_t data = {
                .temp = temperature,
                .hum = humidity,
                .timestamp = xTaskGetTickCount()
            };
            ble_uart_write((uint8_t*)&data, sizeof(data));
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Every 2 seconds
    }
}

// 4. File/Data transfer simulation
void send_large_data(void)
{
    if (!ble_uart_is_connected()) return;
    
    // Simulate sending a large file in chunks
    const char* large_data = "This is a large piece of data that needs to be sent in multiple packets because BLE has packet size limitations...";
    
    size_t total_len = strlen(large_data);
    size_t sent = 0;
    size_t chunk_size = 20; // Send in 20-byte chunks
    
    while (sent < total_len) {
        size_t to_send = (total_len - sent > chunk_size) ? chunk_size : (total_len - sent);
        
        esp_err_t ret = ble_uart_write((uint8_t*)(large_data + sent), to_send);
        if (ret == ESP_OK) {
            sent += to_send;
            ESP_LOGI(TAG, "Sent chunk %zu/%zu", sent, total_len);
        } else {
            ESP_LOGE(TAG, "Failed to send chunk");
            break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay between chunks
    }
}

// 5. Complete initialization example
void complete_setup_example(void)
{
    // Initialize
    esp_err_t ret = ble_uart_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BLE UART init failed");
        return;
    }
    
    // Set RX callback
    ble_uart_set_rx_callback(on_command_received);
    
    // Start service
    ble_uart_start();
    
    // Create tasks
    xTaskCreate(stream_sensor_data_task, "sensor_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "BLE UART setup complete!");
}

void app_main(void)
{
   complete_setup_example();
}