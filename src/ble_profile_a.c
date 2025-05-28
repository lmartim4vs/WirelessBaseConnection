/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "ble_profile_a.h"
#include "ble_profile_b.h"
#include "ble_gatts_server.h"
#include "ble_gap_handler.h"
#include "ble_config.h"
#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "string.h"
#include "inttypes.h"

static const char* TAG = "BLE_PROFILE_A";

// Profile instances
struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
    },
    [PROFILE_B_APP_ID] = {
        .gatts_cb = NULL, // Will be set by ble_profile_b_init()
        .gatts_if = ESP_GATT_IF_NONE,
    },
};

// Prepare write environments
static prepare_type_env_t a_prepare_write_env;

// Characteristic data
static uint8_t char1_str[] = {0x11, 0x22, 0x33};
static esp_gatt_char_prop_t a_property = 0;

static esp_attr_value_t gatts_demo_char1_val = {
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};

static void handle_read_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "Characteristic read, conn_id %d, trans_id %" PRIu32 ", handle %d", 
             param->read.conn_id, param->read.trans_id, param->read.handle);
    
    esp_gatt_rsp_t rsp;
    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
    rsp.attr_value.handle = param->read.handle;
    rsp.attr_value.len = 4;
    rsp.attr_value.value[0] = 0xde;
    rsp.attr_value.value[1] = 0xed;
    rsp.attr_value.value[2] = 0xbe;
    rsp.attr_value.value[3] = 0xef;
    
    esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                ESP_GATT_OK, &rsp);
}

static void handle_write_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "Characteristic write, conn_id %d, trans_id %" PRIu32 ", handle %d", 
             param->write.conn_id, param->write.trans_id, param->write.handle);
    
    if (!param->write.is_prep) {
        ESP_LOGI(TAG, "value len %d, value ", param->write.len);
        ESP_LOG_BUFFER_HEX(TAG, param->write.value, param->write.len);
        
        if (gl_profile_tab[PROFILE_A_APP_ID].descr_handle == param->write.handle && param->write.len == 2) {
            uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
            
            if (descr_value == 0x0001) {
                if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                    ESP_LOGI(TAG, "Notification enable");
                    uint8_t notify_data[NOTIFY_DATA_SIZE];
                    for (int i = 0; i < sizeof(notify_data); ++i) {
                        notify_data[i] = i % 0xff;
                    }
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, 
                                              gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                              sizeof(notify_data), notify_data, false);
                }
            } else if (descr_value == 0x0002) {
                if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE) {
                    ESP_LOGI(TAG, "Indication enable");
                    uint8_t indicate_data[NOTIFY_DATA_SIZE];
                    for (int i = 0; i < sizeof(indicate_data); ++i) {
                        indicate_data[i] = i % 0xff;
                    }
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, 
                                              gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                              sizeof(indicate_data), indicate_data, true);
                }
            } else if (descr_value == 0x0000) {
                ESP_LOGI(TAG, "Notification/Indication disable");
            } else {
                ESP_LOGE(TAG, "Unknown descriptor value");
                ESP_LOG_BUFFER_HEX(TAG, param->write.value, param->write.len);
            }
        }
    }
    example_write_event_env(gatts_if, &a_prepare_write_env, param);
}

static void handle_connect_event(esp_ble_gatts_cb_param_t *param)
{
    esp_ble_conn_update_params_t conn_params = {0};
    memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
    
    /* For the iOS system, please reference the apple official documents about the ble connection parameters restrictions. */
    conn_params.latency = CONN_LATENCY;
    conn_params.max_int = CONN_MAX_INT;    // max_int = 0x20*1.25ms = 40ms
    conn_params.min_int = CONN_MIN_INT;    // min_int = 0x10*1.25ms = 20ms
    conn_params.timeout = CONN_TIMEOUT;    // timeout = 400*10ms = 4000ms
    
    ESP_LOGI(TAG, "Connected, conn_id %u, remote "ESP_BD_ADDR_STR"",
             param->connect.conn_id, ESP_BD_ADDR_HEX(param->connect.remote_bda));
    
    gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
    
    // Start sent the update connection parameters to the peer device.
    esp_ble_gap_update_conn_params(&conn_params);
}

void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "GATT server register, status %d, app_id %d, gatts_if %d", 
                 param->reg.status, param->reg.app_id, gatts_if);
        
        gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        break;
        
    case ESP_GATTS_READ_EVT:
        handle_read_event(gatts_if, param);
        break;
        
    case ESP_GATTS_WRITE_EVT:
        handle_write_event(gatts_if, param);
        break;
        
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(TAG, "Execute write");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
        
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "MTU exchange, MTU %d", param->mtu.mtu);
        break;
        
    case ESP_GATTS_UNREG_EVT:
        break;
        
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "Service create, status %d, service_handle %d", 
                 param->create.status, param->create.service_handle);
        
        gl_profile_tab[PROFILE_A_APP_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_A_APP_ID].service_handle);
        a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_A_APP_ID].service_handle, 
                                                        &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        a_property,
                                                        &gatts_demo_char1_val, NULL);
        if (add_char_ret) {
            ESP_LOGE(TAG, "add char failed, error code =%x", add_char_ret);
        }
        break;
        
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
        
    case ESP_GATTS_ADD_CHAR_EVT: {
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(TAG, "Characteristic add, status %d, attr_handle %d, service_handle %d",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        
        gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        
        esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
        if (get_attr_ret == ESP_FAIL) {
            ESP_LOGE(TAG, "ILLEGAL HANDLE");
        }

        ESP_LOGI(TAG, "the gatts demo char length = %x", length);
        for (int i = 0; i < length; i++) {
            ESP_LOGI(TAG, "prf_char[%x] =%x", i, prf_char[i]);
        }
        
        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_A_APP_ID].service_handle, 
                                                               &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
                                                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
        if (add_descr_ret) {
            ESP_LOGE(TAG, "add char descr failed, error code =%x", add_descr_ret);
        }
        break;
    }
    
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gl_profile_tab[PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(TAG, "Descriptor add, status %d, attr_handle %d, service_handle %d",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
        
    case ESP_GATTS_DELETE_EVT:
        break;
        
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "Service start, status %d, service_handle %d",
                 param->start.status, param->start.service_handle);
        break;
        
    case ESP_GATTS_STOP_EVT:
        break;
        
    case ESP_GATTS_CONNECT_EVT:
        handle_connect_event(param);
        break;
        
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "Disconnected, remote "ESP_BD_ADDR_STR", reason 0x%02x",
                 ESP_BD_ADDR_HEX(param->disconnect.remote_bda), param->disconnect.reason);
        ble_gap_start_advertising();
        break;
        
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "Confirm receive, status %d, attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK) {
            ESP_LOG_BUFFER_HEX(TAG, param->conf.value, param->conf.len);
        }
        break;
        
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}