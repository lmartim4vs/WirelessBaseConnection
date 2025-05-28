/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "ble_profile_b.h"
#include "ble_gatts_server.h"
#include "ble_config.h"
#include "esp_log.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "string.h"
#include "inttypes.h"

static const char* TAG = "BLE_PROFILE_B";

// External reference to global profile table
extern struct gatts_profile_inst gl_profile_tab[PROFILE_NUM];

// Prepare write environment for profile B
static prepare_type_env_t b_prepare_write_env;
static esp_gatt_char_prop_t b_property = 0;

void ble_profile_b_init(void)
{
    gl_profile_tab[PROFILE_B_APP_ID].gatts_cb = gatts_profile_b_event_handler;
}

static void handle_read_event_b(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
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

static void handle_write_event_b(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "Characteristic write, conn_id %d, trans_id %" PRIu32 ", handle %d", 
             param->write.conn_id, param->write.trans_id, param->write.handle);
    
    if (!param->write.is_prep) {
        ESP_LOGI(TAG, "value len %d, value ", param->write.len);
        ESP_LOG_BUFFER_HEX(TAG, param->write.value, param->write.len);
        
        if (gl_profile_tab[PROFILE_B_APP_ID].descr_handle == param->write.handle && param->write.len == 2) {
            uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
            
            if (descr_value == 0x0001) {
                if (b_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                    ESP_LOGI(TAG, "Notification enable");
                    uint8_t notify_data[NOTIFY_DATA_SIZE];
                    for (int i = 0; i < sizeof(notify_data); ++i) {
                        notify_data[i] = i % 0xff;
                    }
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, 
                                              gl_profile_tab[PROFILE_B_APP_ID].char_handle,
                                              sizeof(notify_data), notify_data, false);
                }
            } else if (descr_value == 0x0002) {
                if (b_property & ESP_GATT_CHAR_PROP_BIT_INDICATE) {
                    ESP_LOGI(TAG, "Indication enable");
                    uint8_t indicate_data[NOTIFY_DATA_SIZE];
                    for (int i = 0; i < sizeof(indicate_data); ++i) {
                        indicate_data[i] = i % 0xff;
                    }
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, 
                                              gl_profile_tab[PROFILE_B_APP_ID].char_handle,
                                              sizeof(indicate_data), indicate_data, true);
                }
            } else if (descr_value == 0x0000) {
                ESP_LOGI(TAG, "Notification/Indication disable");
            } else {
                ESP_LOGE(TAG, "Unknown value");
            }
        }
    }
    example_write_event_env(gatts_if, &b_prepare_write_env, param);
}

void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "GATT server register, status %d, app_id %d, gatts_if %d", 
                 param->reg.status, param->reg.app_id, gatts_if);
        
        gl_profile_tab[PROFILE_B_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_B_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_B_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_B_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_B;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_B_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_B);
        break;
        
    case ESP_GATTS_READ_EVT:
        handle_read_event_b(gatts_if, param);
        break;
        
    case ESP_GATTS_WRITE_EVT:
        handle_write_event_b(gatts_if, param);
        break;
        
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(TAG, "Execute write");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&b_prepare_write_env, param);
        break;
        
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "MTU exchange, MTU %d", param->mtu.mtu);
        break;
        
    case ESP_GATTS_UNREG_EVT:
        break;
        
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "Service create, status %d, service_handle %d", 
                 param->create.status, param->create.service_handle);
        
        gl_profile_tab[PROFILE_B_APP_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_B_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_B_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_B;

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_B_APP_ID].service_handle);
        b_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_B_APP_ID].service_handle, 
                                                        &gl_profile_tab[PROFILE_B_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        b_property,
                                                        NULL, NULL);
        if (add_char_ret) {
            ESP_LOGE(TAG, "add char failed, error code =%x", add_char_ret);
        }
        break;
        
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
        
    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(TAG, "Characteristic add, status %d, attr_handle %d, service_handle %d",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);

        gl_profile_tab[PROFILE_B_APP_ID].char_handle = param->add_char.attr_handle;
        gl_profile_tab[PROFILE_B_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_B_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        
        esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_B_APP_ID].service_handle, 
                                     &gl_profile_tab[PROFILE_B_APP_ID].descr_uuid,
                                     ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                     NULL, NULL);
        break;
        
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gl_profile_tab[PROFILE_B_APP_ID].descr_handle = param->add_char_descr.attr_handle;
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
        ESP_LOGI(TAG, "Connected, conn_id %d, remote "ESP_BD_ADDR_STR"",
                 param->connect.conn_id, ESP_BD_ADDR_HEX(param->connect.remote_bda));
        gl_profile_tab[PROFILE_B_APP_ID].conn_id = param->connect.conn_id;
        break;
        
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "Confirm receive, status %d, attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK) {
            ESP_LOG_BUFFER_HEX(TAG, param->conf.value, param->conf.len);
        }
        break;
        
    case ESP_GATTS_DISCONNECT_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}