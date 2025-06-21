#include "esp_bt.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "esp_sleep.h"
#include "esp_mac.h"
#include "generateRandom.h"
#include "config.h"
#include "nvs.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "sdkconfig.h"

#define GATTS_TAG "GATTS_DEMO"
#define CLIENT_CONNECTED_BIT BIT0
#define CLIENT_WRITING_READING_BIT BIT1

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event,
                                          esp_gatt_if_t gatts_if,
                                          esp_ble_gatts_cb_param_t *param);
static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event,
                                          esp_gatt_if_t gatts_if,
                                          esp_ble_gatts_cb_param_t *param);

typedef struct
{
    uint16_t id;
    uint8_t mac[6];
    uint8_t transport_layer;
    uint8_t id_protocol;
    uint16_t length;
} Header;

static Config config_instance; 
static uint8_t *pMessage;     
static EventGroupHandle_t client_connected;
static EventGroupHandle_t client_writing_reading;

void request_config();
void add_mac(Header *header_instance);
void make_header(Header *header_instance);
void generate_message(Header *header_instance, uint8_t *message);
void task(void *param);
void save_nvs();
void load_nvs();

static bool stop_config = false;

#define GATTS_SERVICE_UUID_TEST_A 0x00FF
#define GATTS_CHAR_UUID_TEST_A 0xFF01
#define GATTS_DESCR_UUID_TEST_A 0x3333
#define GATTS_NUM_HANDLE_TEST_A 4
#define GATTS_SERVICE_UUID_TEST_B 0x00EE
#define GATTS_CHAR_UUID_TEST_B 0xEE01
#define GATTS_DESCR_UUID_TEST_B 0x2222
#define GATTS_NUM_HANDLE_TEST_B 4
#define TEST_DEVICE_NAME "ESP_GATTS_DEMO"
#define TEST_MANUFACTURER_DATA_LEN 17
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40
#define PREPARE_BUF_MAX_SIZE 1024

static uint8_t char1_str[] = {0x11, 0x22, 0x33};
static esp_gatt_char_prop_t a_property = 0;
static esp_gatt_char_prop_t b_property = 0;

static esp_attr_value_t gatts_demo_char1_val = {
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(char1_str),
    .attr_value = char1_str,
};

static uint8_t adv_config_done = 0;
#define adv_config_flag (1 << 0)
#define scan_rsp_config_flag (1 << 1)

#ifdef CONFIG_SET_RAW_ADV_DATA
static uint8_t raw_adv_data[] = {
    0x02, 0x01, 0x06, 
    0x02, 0x0a, 0xeb, 
    0x03, 0x03, 0xab, 0xcd, 
};
static uint8_t raw_scan_rsp_data[] =
    {0x0f, 0x09, 0x45, 0x53, 0x50, 0x5f, 0x47, 0x41,
     0x54, 0x54, 0x53, 0x5f, 0x44, 0x45, 0x4d, 0x4f};
#else
static uint8_t adv_service_uuid128[32] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
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
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .appearance = 0x00,
    .manufacturer_len = 0,       
    .p_manufacturer_data = NULL, 
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
#endif /* CONFIG_SET_RAW_ADV_DATA */

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

#define PROFILE_NUM 2
#define PROFILE_A_APP_ID 0
#define PROFILE_B_APP_ID 1

struct gatts_profile_inst
{
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

static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
    },
    [PROFILE_B_APP_ID] = {
        .gatts_cb = gatts_profile_b_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
    },
};

typedef struct
{
    uint8_t *prepare_buf;
    int prepare_len;
} prepare_type_env_t;

static prepare_type_env_t a_prepare_write_env;
static prepare_type_env_t b_prepare_write_env;

void example_write_event_env(esp_gatt_if_t gatts_if,
                             prepare_type_env_t *prepare_write_env,
                             esp_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env,
                                  esp_ble_gatts_cb_param_t *param);

static void gap_event_handler(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
#ifdef CONFIG_SET_RAW_ADV_DATA
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
#else
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
#endif
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GATTS_TAG, "Advertising start failed");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GATTS_TAG, "Advertising stop failed");
        }
        else
        {
            ESP_LOGI(GATTS_TAG, "Stop adv successfully");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(
            GATTS_TAG,
            "update connection params status = %d, min_int = %d, max_int = "
            "%d,conn_int = %d,latency = %d, timeout = %d",
            param->update_conn_params.status, param->update_conn_params.min_int,
            param->update_conn_params.max_int, param->update_conn_params.conn_int,
            param->update_conn_params.latency, param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
        ESP_LOGI(GATTS_TAG, "packet length updated: rx = %d, tx = %d, status = %d",
                 param->pkt_data_length_cmpl.params.rx_len,
                 param->pkt_data_length_cmpl.params.tx_len,
                 param->pkt_data_length_cmpl.status);
        break;
    default:
        break;
    }
}

static int i = 0;
void example_write_event_env(esp_gatt_if_t gatts_if,
                             prepare_type_env_t *prepare_write_env,
                             esp_ble_gatts_cb_param_t *param)
{
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp)
    {
        if (param->write.is_prep)
        {
            if (param->write.offset > PREPARE_BUF_MAX_SIZE)
            {
                status = ESP_GATT_INVALID_OFFSET;
            }
            else if ((param->write.offset + param->write.len) >
                     PREPARE_BUF_MAX_SIZE)
            {
                status = ESP_GATT_INVALID_ATTR_LEN;
            }
            if (status == ESP_GATT_OK && prepare_write_env->prepare_buf == NULL)
            {
                prepare_write_env->prepare_buf =
                    (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL)
                {
                    ESP_LOGE(GATTS_TAG, "Gatt_server prep no mem");
                    status = ESP_GATT_NO_RESOURCES;
                }
            }
            esp_gatt_rsp_t *gatt_rsp =
                (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            if (gatt_rsp)
            {
                gatt_rsp->attr_value.len = param->write.len;
                gatt_rsp->attr_value.handle = param->write.handle;
                gatt_rsp->attr_value.offset = param->write.offset;
                gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
                memcpy(gatt_rsp->attr_value.value, param->write.value,
                       param->write.len);
                esp_err_t response_err = esp_ble_gatts_send_response(
                    gatts_if, param->write.conn_id, param->write.trans_id, status,
                    gatt_rsp);
                if (response_err != ESP_OK)
                {
                    ESP_LOGE(GATTS_TAG, "Send response error\n");
                }
                free(gatt_rsp);
            }
            else
            {
                ESP_LOGE(GATTS_TAG,
                         "malloc failed, no resource to send response error\n");
                status = ESP_GATT_NO_RESOURCES;
            }
            if (status != ESP_GATT_OK)
            {
                return;
            }
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                   param->write.value, param->write.len);
            prepare_write_env->prepare_len += param->write.len;
        }
        else
        {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                        param->write.trans_id, status, NULL);
            if (i == 0)
            {
                i += 1;
                return;
            }
            xEventGroupSetBits(client_writing_reading, CLIENT_WRITING_READING_BIT);
            stop_config = true;
            int offset = 0;
            mempcpy(&config_instance.status, param->write.value + offset, 1);
            offset += 1;
            mempcpy(&config_instance.ID_Protocol, param->write.value + offset, 1);
            offset += 1;
            mempcpy(&config_instance.BMI270_Sampling, param->write.value + offset, 4);
            offset += 4;
            mempcpy(&config_instance.BMI270_Acc_Sensibility, param->write.value + offset, 4);
            offset += 4;
            mempcpy(&config_instance.BMI270_Gyro_Sensibility, param->write.value + offset, 4);
            offset += 4;
            mempcpy(&config_instance.BME688_Sampling, param->write.value + offset, 4);
            offset += 4;
            mempcpy(&config_instance.Discontinuous_Time, param->write.value + offset, 4);
            offset += 4;
            mempcpy(&config_instance.Port_TCP, param->write.value + offset, 4);
            offset += 4;
            mempcpy(&config_instance.Port_UDP, param->write.value + offset, 4);
            offset += 4;
            mempcpy(&config_instance.Host_Ip_Addr, param->write.value + offset, 4);
            offset += 4;
            mempcpy(&config_instance.Ssid, param->write.value + offset, 10);
            offset += 10;
            mempcpy(&config_instance.Pass, param->write.value + offset, 10);
            save_nvs();
            xEventGroupClearBits(client_writing_reading, CLIENT_WRITING_READING_BIT);
        }
    }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env,
                                  esp_ble_gatts_cb_param_t *param)
{
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC)
    {
        ESP_LOG_BUFFER_HEX(GATTS_TAG, prepare_write_env->prepare_buf,
                           prepare_write_env->prepare_len);
    }
    else
    {
        ESP_LOGI(GATTS_TAG, "ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf)
    {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event,
                                          esp_gatt_if_t gatts_if,
                                          esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d",
                 param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 =
            GATTS_SERVICE_UUID_TEST_A;
        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
        if (set_dev_name_ret)
        {
            ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x",
                     set_dev_name_ret);
        }
#ifdef CONFIG_SET_RAW_ADV_DATA
        esp_err_t raw_adv_ret =
            esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
        if (raw_adv_ret)
        {
            ESP_LOGE(GATTS_TAG, "config raw adv data failed, error code = %x ",
                     raw_adv_ret);
        }
        adv_config_done |= adv_config_flag;
        esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(
            raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
        if (raw_scan_ret)
        {
            ESP_LOGE(GATTS_TAG, "config raw scan rsp data failed, error code = %x",
                     raw_scan_ret);
        }
        adv_config_done |= scan_rsp_config_flag;
#else
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret)
        {
            ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret)
        {
            ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x",
                     ret);
        }
        adv_config_done |= scan_rsp_config_flag;
#endif
        esp_ble_gatts_create_service(gatts_if,
                                     &gl_profile_tab[PROFILE_A_APP_ID].service_id,
                                     GATTS_NUM_HANDLE_TEST_A);
        break;
    case ESP_GATTS_READ_EVT:
    {
        ESP_LOGI(GATTS_TAG,
                 "GATT_READ_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d",
                 param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        memcpy(&rsp.attr_value.len, pMessage + 10, 2); 
        memcpy(&rsp.attr_value.value, pMessage, rsp.attr_value.len); 
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id,
                                    param->read.trans_id, ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT:
    {
        ESP_LOGI(GATTS_TAG,
                 "GATT_WRITE_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d",
                 param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep)
        {
            ESP_LOGI(GATTS_TAG,
                     "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            ESP_LOG_BUFFER_HEX(GATTS_TAG, param->write.value, param->write.len);
            if (gl_profile_tab[PROFILE_A_APP_ID].descr_handle ==
                    param->write.handle &&
                param->write.len == 2)
            {
                uint16_t descr_value =
                    param->write.value[1] << 8 | param->write.value[0];
                if (descr_value == 0x0001)
                {
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
                    {
                        ESP_LOGI(GATTS_TAG, "notify enable");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++i)
                        {
                            notify_data[i] = i % 0xff;
                        }
                        esp_ble_gatts_send_indicate(
                            gatts_if, param->write.conn_id,
                            gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                            sizeof(notify_data), notify_data, false);
                    }
                }
                else if (descr_value == 0x0002)
                {
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE)
                    {
                        ESP_LOGI(GATTS_TAG, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i % 0xff;
                        }
                        esp_ble_gatts_send_indicate(
                            gatts_if, param->write.conn_id,
                            gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                            sizeof(indicate_data), indicate_data, true);
                    }
                }
                else if (descr_value == 0x0000)
                {
                    ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
                }
                else
                {
                    ESP_LOGE(GATTS_TAG, "unknown descr value");
                    ESP_LOG_BUFFER_HEX(GATTS_TAG, param->write.value, param->write.len);
                }
            }
        }
        example_write_event_env(gatts_if, &a_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                    param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d",
                 param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_A_APP_ID].service_handle =
            param->create.service_handle;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.uuid.uuid16 =
            GATTS_CHAR_UUID_TEST_A;
        esp_ble_gatts_start_service(
            gl_profile_tab[PROFILE_A_APP_ID].service_handle);
        a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE |
                     ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_char_ret =
            esp_ble_gatts_add_char(gl_profile_tab[PROFILE_A_APP_ID].service_handle,
                                   &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
                                   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                   a_property, &gatts_demo_char1_val, NULL);
        if (add_char_ret)
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
    {
        uint16_t length = 0;
        const uint8_t *prf_char;
        ESP_LOGI(GATTS_TAG,
                 "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d",
                 param->add_char.status, param->add_char.attr_handle,
                 param->add_char.service_handle);
        gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 =
            ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(
            param->add_char.attr_handle, &length, &prf_char);
        if (get_attr_ret == ESP_FAIL)
        {
            ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
        }
        ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x", length);
        for (int i = 0; i < length; i++)
        {
            ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x", i, prf_char[i]);
        }
        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(
            gl_profile_tab[PROFILE_A_APP_ID].service_handle,
            &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
        if (add_descr_ret)
        {
            ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x",
                     add_descr_ret);
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gl_profile_tab[PROFILE_A_APP_ID].descr_handle =
            param->add_char_descr.attr_handle;
        ESP_LOGI(GATTS_TAG,
                 "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d",
                 param->add_char_descr.status, param->add_char_descr.attr_handle,
                 param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
    {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        conn_params.latency = 0;
        conn_params.max_int = 0x20; 
        conn_params.min_int = 0x10; 
        conn_params.timeout = 400;  
        ESP_LOGI(GATTS_TAG,
                 "ESP_GATTS_CONNECT_EVT, conn_id %d, remote "
                 "%02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id, param->connect.remote_bda[0],
                 param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4],
                 param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
        xEventGroupSetBits(client_connected, CLIENT_CONNECTED_BIT);
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x",
                 param->disconnect.reason);
        xEventGroupClearBits(client_connected, CLIENT_CONNECTED_BIT);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d",
                 param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK)
        {
            ESP_LOG_BUFFER_HEX(GATTS_TAG, param->conf.value, param->conf.len);
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

static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event,
                                          esp_gatt_if_t gatts_if,
                                          esp_ble_gatts_cb_param_t *param)
{
}

static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        }
        else
        {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d",
                     param->reg.app_id, param->reg.status);
            return;
        }
    }
    do
    {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            if (gatts_if == ESP_GATT_IF_NONE ||
                gatts_if == gl_profile_tab[idx].gatts_if)
            {
                if (gl_profile_tab[idx].gatts_cb)
                {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s", __func__,
                 esp_err_to_name(ret));
        return;
    }
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s", __func__,
                 esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s", __func__,
                 esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s", __func__,
                 esp_err_to_name(ret));
        return;
    }
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(PROFILE_B_APP_ID);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
        return;
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret)
    {
        ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x",
                 local_mtu_ret);
    }
    client_connected = xEventGroupCreate();
    client_writing_reading = xEventGroupCreate();
    esp_ble_gatts_register_callback(gatts_event_handler);
    xTaskCreate(&task, "main_task", 2048, NULL, 5, NULL);
    return;
}

void request_config()
{
    char *message = "CONFIG";
    int len = strlen(message);
    esp_err_t err = esp_ble_gatts_send_indicate(
        gl_profile_tab[PROFILE_A_APP_ID].gatts_if,
        gl_profile_tab[PROFILE_A_APP_ID].conn_id,
        gl_profile_tab[PROFILE_A_APP_ID].char_handle,
        len, (uint8_t *)message, false);
    if (err != ESP_OK)
    {
        ESP_LOGE(GATTS_TAG, "Failed to send indicate: %s", esp_err_to_name(err));
    }
}

void add_mac(Header *header_instance)
{
    esp_err_t result = esp_read_mac(header_instance->mac, ESP_MAC_BT);
    if (result != ESP_OK)
    {
        printf("Failed to read MAC address\n");
        return;
    }
}

void make_header(Header *header_instance)
{
    srand(time(NULL));
    header_instance->id = generateId();
    header_instance->transport_layer = config_instance.status;
    header_instance->id_protocol = config_instance.ID_Protocol;
    switch (header_instance->id_protocol)
    {
    case 1:
        header_instance->length = 17;
        break;
    case 2:
        header_instance->length = 27;
        break;
    case 3:
        header_instance->length = 31;
        break;
    case 4:
        header_instance->length = 55;
        break;
    }
    printf("header: %d; %02x; %02x; %02x; %02x; %02x; %02x; %d; %d; %d;\n", header_instance->id, header_instance->mac[0], header_instance->mac[1], header_instance->mac[2], header_instance->mac[3], header_instance->mac[4], header_instance->mac[5], header_instance->transport_layer, header_instance->id_protocol, header_instance->length);
}

void generate_message(Header *header_instance, uint8_t *message)
{
#define HEADER_SIZE 12
    int offset = 0;
    srand(time(NULL));
    memcpy(message + offset, &header_instance->id, 2);
    offset += 2;
    memcpy(message + offset, &header_instance->mac, 6);
    offset += 6;
    memcpy(message + offset, &header_instance->transport_layer, 1);
    offset += 1;
    memcpy(message + offset, &header_instance->id_protocol, 1);
    offset += 1;
    memcpy(message + offset, &header_instance->length, 2);
    offset += 2;
    offset = HEADER_SIZE;
    printf("------------------------------------------------------------\n");
    unsigned char batt = generateBatteryLevel();
    memcpy(message + offset, &batt, 1);
    offset += 1;
    printf("batt: %d\n", batt);
    time_t current_time = time(NULL);
    uint32_t timeStamp = (uint32_t)current_time;
    memcpy(message + offset, &timeStamp, 4);
    offset += 4;
    printf("timestamp: %" PRIu32 "\n", timeStamp);
    int protocol = header_instance->id_protocol;
    if (protocol > 1)
    {
        int temperature = generateTemperature();
        int press = generatePressure();
        int hum = generateHumidity();
        int co = generateCO();
        memcpy(message + offset, &temperature, 1);
        offset += 1;
        printf("temperature: %d\n", temperature);
        memcpy(message + offset, &press, 4);
        offset += 4;
        printf("press: %d\n", press);
        memcpy(message + offset, &hum, 1);
        offset += 1;
        printf("hum: %d\n", hum);
        memcpy(message + offset, &co, 4);
        offset += 4;
        printf("co: %d\n", co);
    }
    if (protocol > 2)
    {
        float ampx = generateAmpx();
        float ampy = generateAmpy();
        float ampz = generateAmpz();
        float rms = generateRMS(ampx, ampy, ampz);
        memcpy(message + offset, &rms, 4);
        offset += 4;
        printf("rms: %f\n", rms);
    }
    if (protocol > 3)
    {
        float ampx = generateAmpx();
        float freqx = generateFreqx();
        float ampy = generateAmpy();
        float freqy = generateFreqy();
        float ampz = generateAmpz();
        float freqz = generateFreqz();
        memcpy(message + offset, &ampx, 4);
        offset += 4;
        printf("ampx: %f\n", ampx);
        memcpy(message + offset, &freqx, 4);
        offset += 4;
        printf("freqx: %f\n", freqx);
        memcpy(message + offset, &ampy, 4);
        offset += 4;
        printf("ampy: %f\n", ampy);
        memcpy(message + offset, &freqy, 4);
        offset += 4;
        printf("freqy: %f\n", freqy);
        memcpy(message + offset, &ampz, 4);
        offset += 4;
        printf("ampz: %f\n", ampz);
        memcpy(message + offset, &freqz, 4);
        offset += 4;
        printf("freqz: %f\n", freqz);
    }
    printf("------------------------------------------------------------\n");
}

void save_nvs()
{
    Write_NVS_int(config_instance.status, 1);
    Write_NVS_int(config_instance.ID_Protocol, 2);
    Write_NVS_int(config_instance.BMI270_Sampling, 3);
    Write_NVS_int(config_instance.BMI270_Acc_Sensibility, 4);
    Write_NVS_int(config_instance.BMI270_Gyro_Sensibility, 5);
    Write_NVS_int(config_instance.BME688_Sampling, 6);
    Write_NVS_int(config_instance.Discontinuous_Time, 7);
    Write_NVS_int(config_instance.Port_TCP, 8);
    Write_NVS_int(config_instance.Port_UDP, 9);
    Write_NVS_int(config_instance.Host_Ip_Addr, 10);
    Write_NVS_string(config_instance.Ssid, 11);
    Write_NVS_string(config_instance.Pass, 12);
}

void load_nvs()
{
    Read_NVS_int(&config_instance.status, 1);
    Read_NVS_int(&config_instance.ID_Protocol, 2);
    Read_NVS_int(&config_instance.BMI270_Sampling, 3);
    Read_NVS_int(&config_instance.BMI270_Acc_Sensibility, 4);
    Read_NVS_int(&config_instance.BMI270_Gyro_Sensibility, 5);
    Read_NVS_int(&config_instance.BME688_Sampling, 6);
    Read_NVS_int(&config_instance.Discontinuous_Time, 7);
    Read_NVS_int(&config_instance.Port_TCP, 8);
    Read_NVS_int(&config_instance.Port_UDP, 9);
    Read_NVS_int(&config_instance.Host_Ip_Addr, 10);
    size_t len;
    len = sizeof(config_instance.Ssid);
    Read_NVS_string(config_instance.Ssid, &len, 11);
    len = sizeof(config_instance.Pass);
    Read_NVS_string(config_instance.Pass, &len, 12);
}

void task(void *param)
{
    Header header_instance;
    add_mac(&header_instance);
    while (1)
    {
        printf("Esperando conexión...\n");
        xEventGroupWaitBits(client_connected, CLIENT_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        ESP_LOGI("NOTIFY_TASK", "Connected to a BLE client, starting notifications.");
        printf("Cliente conectado\n");
        while (xEventGroupGetBits(client_connected) & CLIENT_CONNECTED_BIT)
        {
            load_nvs();
            printf("------------------------------------------------------------\n");
            printf("Data inicial:\n");
            printf("%" PRId32 "\n", config_instance.status);
            printf("%" PRId32 "\n", config_instance.ID_Protocol);
            printf("%ld\n", config_instance.BMI270_Sampling);
            printf("%ld\n", config_instance.BMI270_Acc_Sensibility);
            printf("%ld\n", config_instance.BMI270_Gyro_Sensibility);
            printf("%ld\n", config_instance.BME688_Sampling);
            printf("%ld\n", config_instance.Discontinuous_Time);
            printf("%ld\n", config_instance.Port_TCP);
            printf("%ld\n", config_instance.Port_UDP);
            printf("%ld\n", config_instance.Host_Ip_Addr);
            printf("%s\n", config_instance.Ssid);
            printf("%s\n", config_instance.Pass);
            printf("------------------------------------------------------------\n");
            config_instance.status = 0; 
            if (config_instance.status == 0)
            {
                while (!stop_config && (xEventGroupGetBits(client_connected) & CLIENT_CONNECTED_BIT))
                {
                    request_config();
                    printf("Config requested\n");
                    vTaskDelay(1000 / portTICK_PERIOD_MS); 
                }
            }
            while (xEventGroupGetBits(client_writing_reading) & CLIENT_WRITING_READING_BIT)
            {
                vTaskDelay(pdMS_TO_TICKS(100)); 
            }
            printf("Received and written config\n");            
            load_nvs();
            printf("------------------------------------------------------------\n");
            printf("Data recibida:\n");
            printf("%" PRId32 "\n", config_instance.status);
            printf("%" PRId32 "\n", config_instance.ID_Protocol);
            printf("%ld\n", config_instance.BMI270_Sampling);
            printf("%ld\n", config_instance.BMI270_Acc_Sensibility);
            printf("%ld\n", config_instance.BMI270_Gyro_Sensibility);
            printf("%ld\n", config_instance.BME688_Sampling);
            printf("%ld\n", config_instance.Discontinuous_Time);
            printf("%ld\n", config_instance.Port_TCP);
            printf("%ld\n", config_instance.Port_UDP);
            printf("%ld\n", config_instance.Host_Ip_Addr);
            printf("%s\n", config_instance.Ssid);
            printf("%s\n", config_instance.Pass);
            printf("------------------------------------------------------------\n");
            make_header(&header_instance);
            pMessage = malloc(header_instance.length * sizeof(uint8_t));
            generate_message(&header_instance, pMessage);
            printf("Message is:\n");
            for (int i = 0; i < header_instance.length; i++)
            {
                printf("%02X", pMessage[i]); 
            }
            printf("\n");
            esp_err_t err = esp_ble_gatts_send_indicate(gl_profile_tab[PROFILE_A_APP_ID].gatts_if,
                                                        gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                        gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                                        header_instance.length, (uint8_t *)pMessage, false);
            if (err != ESP_OK)
            {
                ESP_LOGE(GATTS_TAG, "Failed to send indicate: %s", esp_err_to_name(err));
            }
            vTaskDelay(5000 / portTICK_PERIOD_MS); 
            header_instance.id = 0;
            header_instance.transport_layer = 0;
            header_instance.id_protocol = 0;
            header_instance.length = 0;
            free(pMessage);
            stop_config = false;
            if (config_instance.status == 31)
            {
                esp_err_t ret = esp_bluedroid_disable();
                if (ret)
                {
                    ESP_LOGE(GATTS_TAG, "Failed to disable Bluedroid: %s", esp_err_to_name(ret));
                }
                ret = esp_bt_controller_disable();
                if (ret)
                {
                    ESP_LOGE(GATTS_TAG, "Failed to disable Bluetooth controller: %s", esp_err_to_name(ret));
                }
                esp_sleep_enable_timer_wakeup(5000000); 
                esp_deep_sleep_start();
            }
        }
    }
}
