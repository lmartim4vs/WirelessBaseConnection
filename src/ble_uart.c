/*
 * Simple BLE UART Service Implementation
 */

#include "ble_uart.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "BLE_UART";

// GATT Server state
typedef struct {
    esp_gatt_if_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    uint16_t tx_char_handle;
    uint16_t rx_char_handle;
    uint16_t tx_descr_handle;
    bool connected;
    ble_uart_rx_callback_t rx_callback;
} ble_uart_state_t;

static ble_uart_state_t g_uart_state = {
    .gatts_if = ESP_GATT_IF_NONE,
    .app_id = 0,
    .conn_id = 0,
    .connected = false,
    .rx_callback = NULL
};

// Advertising data
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising start failed");
        } else {
            ESP_LOGI(TAG, "Advertising started successfully");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising stop failed");
        } else {
            ESP_LOGI(TAG, "Advertising stopped successfully");
        }
        break;
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "GATT server registered, app_id %d", param->reg.app_id);
        g_uart_state.gatts_if = gatts_if;
        
        // Set device name
        esp_ble_gap_set_device_name(BLE_UART_DEVICE_NAME);
        
        // Configure advertising data
        esp_ble_gap_config_adv_data(&adv_data);
        
        // Create service
        esp_gatt_srvc_id_t service_id = {
            .is_primary = true,
            .id.inst_id = 0x00,
            .id.uuid.len = ESP_UUID_LEN_32,
            .id.uuid.uuid.uuid32 = BLE_UART_SERVICE_UUID,
        };
        esp_ble_gatts_create_service(gatts_if, &service_id, 6); // Need 6 handles
        break;

    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "Service created, service_handle %d", param->create.service_handle);
        g_uart_state.service_handle = param->create.service_handle;
        
        // Start service
        esp_ble_gatts_start_service(g_uart_state.service_handle);
        
        // Add TX characteristic (for sending data TO client)
        esp_bt_uuid_t tx_char_uuid = {
            .len = ESP_UUID_LEN_32,
            .uuid.uuid32 = BLE_UART_TX_CHAR_UUID,
        };
        esp_ble_gatts_add_char(g_uart_state.service_handle, &tx_char_uuid,
                              ESP_GATT_PERM_READ,
                              ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                              NULL, NULL);
        break;

    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(TAG, "Characteristic added, attr_handle %d", param->add_char.attr_handle);
        
        if (g_uart_state.tx_char_handle == 0) {
            // This is the TX characteristic
            g_uart_state.tx_char_handle = param->add_char.attr_handle;
            
            // Add descriptor for TX characteristic (for notifications)
            esp_bt_uuid_t descr_uuid = {
                .len = ESP_UUID_LEN_16,
                .uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,
            };
            esp_ble_gatts_add_char_descr(g_uart_state.service_handle, &descr_uuid,
                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
        } else {
            // This is the RX characteristic
            g_uart_state.rx_char_handle = param->add_char.attr_handle;
            ESP_LOGI(TAG, "BLE UART service setup complete");
        }
        break;

    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        ESP_LOGI(TAG, "Descriptor added, attr_handle %d", param->add_char_descr.attr_handle);
        g_uart_state.tx_descr_handle = param->add_char_descr.attr_handle;
        
        // Add RX characteristic (for receiving data FROM client)
        esp_bt_uuid_t rx_char_uuid = {
            .len = ESP_UUID_LEN_32,
            .uuid.uuid32 = BLE_UART_RX_CHAR_UUID,
        };
        esp_ble_gatts_add_char(g_uart_state.service_handle, &rx_char_uuid,
                              ESP_GATT_PERM_WRITE,
                              ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR,
                              NULL, NULL);
        break;

    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(TAG, "Client connected, conn_id %d", param->connect.conn_id);
        g_uart_state.conn_id = param->connect.conn_id;
        g_uart_state.connected = true;
        
        // Stop advertising when connected
        esp_ble_gap_stop_advertising();
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "Client disconnected, conn_id %d", param->disconnect.conn_id);
        g_uart_state.connected = false;
        
        // Restart advertising
        esp_ble_gap_start_advertising(&adv_params);
        break;

    case ESP_GATTS_WRITE_EVT:
        ESP_LOGI(TAG, "Data received, handle %d, len %d", param->write.handle, param->write.len);
        
        if (param->write.handle == g_uart_state.rx_char_handle) {
            // Data received on RX characteristic
            if (g_uart_state.rx_callback) {
                g_uart_state.rx_callback(param->write.value, param->write.len);
            }
            ESP_LOG_BUFFER_HEX(TAG, param->write.value, param->write.len);
        } else if (param->write.handle == g_uart_state.tx_descr_handle) {
            // Client enabling/disabling notifications
            uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
            if (descr_value == 0x0001) {
                ESP_LOGI(TAG, "Notifications enabled");
            } else {
                ESP_LOGI(TAG, "Notifications disabled");
            }
        }
        
        // Send response if needed
        if (param->write.need_rsp) {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id,
                                       ESP_GATT_OK, NULL);
        }
        break;

    case ESP_GATTS_READ_EVT:
        ESP_LOGI(TAG, "Read request, handle %d", param->read.handle);
        
        // Send empty response for now
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 0;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                   ESP_GATT_OK, &rsp);
        break;

    default:
        break;
    }
}

esp_err_t ble_uart_init(void)
{
    esp_err_t ret;

    // Initialize Bluetooth controller
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "Initialize controller failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "Enable controller failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Init bluetooth failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Enable bluetooth failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register callbacks
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "GATTS register error: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "GAP register error: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register application
    ret = esp_ble_gatts_app_register(g_uart_state.app_id);
    if (ret) {
        ESP_LOGE(TAG, "GATTS app register error: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "BLE UART initialized");
    return ESP_OK;
}

esp_err_t ble_uart_start(void)
{
    if (g_uart_state.gatts_if == ESP_GATT_IF_NONE) {
        ESP_LOGE(TAG, "BLE UART not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting BLE UART service...");
    return ESP_OK;
}

esp_err_t ble_uart_stop(void)
{
    return esp_ble_gap_stop_advertising();
}

esp_err_t ble_uart_write(uint8_t* data, uint16_t len)
{
    if (!g_uart_state.connected) {
        ESP_LOGW(TAG, "No client connected, cannot send data");
        return ESP_ERR_INVALID_STATE;
    }

    if (len > BLE_UART_MAX_DATA_LEN) {
        ESP_LOGE(TAG, "Data too long (%d > %d)", len, BLE_UART_MAX_DATA_LEN);
        return ESP_ERR_INVALID_SIZE;
    }

    ESP_LOGI(TAG, "Sending %d bytes via BLE UART TX", len);
    ESP_LOG_BUFFER_HEX(TAG, data, len);

    return esp_ble_gatts_send_indicate(g_uart_state.gatts_if, g_uart_state.conn_id, 
                                      g_uart_state.tx_char_handle, len, data, false);
}

void ble_uart_set_rx_callback(ble_uart_rx_callback_t callback)
{
    g_uart_state.rx_callback = callback;
    ESP_LOGI(TAG, "RX callback %s", callback ? "set" : "cleared");
}

bool ble_uart_is_connected(void)
{
    return g_uart_state.connected;
}

const char* ble_uart_get_status_string(void)
{
    if (g_uart_state.connected) {
        return "Connected";
    } else if (g_uart_state.gatts_if != ESP_GATT_IF_NONE) {
        return "Advertising";
    } else {
        return "Not initialized";
    }
}