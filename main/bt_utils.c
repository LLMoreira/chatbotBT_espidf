#include "bt_utils.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "sys/time.h"
#include "esp_spp_api.h"

#define SPP_TAG "SPP_ACCEPTOR_DEMO"
#define SPP_SERVER_NAME "SPP_SERVER"
#define EXAMPLE_DEVICE_NAME "CPS_CONNECTIONS"

const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
const bool esp_spp_enable_l2cap_ertm = true;
struct timeval time_new, time_old;
long data_num = 0;

const esp_spp_sec_t sec_mask;
const esp_spp_role_t role_slave;

char *bda2str(uint8_t *bda, char *str, size_t size) {
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

void trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return;

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    *(end + 1) = 0;
}

void spp_send_data(uint32_t handle, const char *data) {
    esp_err_t ret = esp_spp_write(handle, strlen(data), (uint8_t *)data);
    if (ret != ESP_OK) {
        ESP_LOGE(SPP_TAG, "Error sending data: %s", esp_err_to_name(ret));
    }
}

void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    char bda_str[18] = {0};
    char received_data[256];

    switch (event) {
        case ESP_SPP_INIT_EVT:
            if (param->init.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
                esp_spp_start_srv(sec_mask, role_slave, 0, SPP_SERVER_NAME);
            } else {
                ESP_LOGE(SPP_TAG, "ESP_SPP_INIT_EVT status:%d", param->init.status);
            }
            break;
        case ESP_SPP_DISCOVERY_COMP_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
            break;
        case ESP_SPP_OPEN_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT");
            break;
        case ESP_SPP_CLOSE_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_CLOSE_EVT status:%d handle:%"PRIu32" close_by_remote:%d",
                     param->close.status, param->close.handle, param->close.async);
            break;
        case ESP_SPP_START_EVT:
            if (param->start.status == ESP_SPP_SUCCESS) {
                ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT handle:%"PRIu32" sec_id:%d scn:%d", param->start.handle,
                         param->start.sec_id, param->start.scn);
                esp_bt_dev_set_device_name(EXAMPLE_DEVICE_NAME);
                esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            } else {
                ESP_LOGE(SPP_TAG, "ESP_SPP_START_EVT status:%d", param->start.status);
            }
            break;
        case ESP_SPP_CL_INIT_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT");
            break;
        case ESP_SPP_DATA_IND_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT len:%d handle:%"PRIu32, param->data_ind.len, param->data_ind.handle);

            if (param->data_ind.len < sizeof(received_data) - 1) {
                memcpy(received_data, param->data_ind.data, param->data_ind.len);
                received_data[param->data_ind.len] = '\0';
                ESP_LOGI(SPP_TAG, "Received data: %s", received_data);

                trim_whitespace(received_data);
                ESP_LOGI(SPP_TAG, "Trimmed data: %s", received_data);

                if (strcmp(received_data, "CONFIG") == 0 || strcmp(received_data, "config") == 0) {
                    const char *response = "1-Config. atual 2-Criar config. 3-Apagar config.\n";
                    spp_send_data(param->data_ind.handle, response);
                } else if (strcmp(received_data, "1") == 0) {
                    const char *response = readFileConfig();
                    spp_send_data(param->data_ind.handle, response);
                } else if (strcmp(received_data, "2") == 0) {
                    ESP_LOGI(SPP_TAG, "Criando o arquivo de configuração");
                    createFileConfig();
                    const char *response = "Escreva o ip:\n";
                    spp_send_data(param->data_ind.handle, response);
                } else if (strcmp(received_data, "3") == 0) {
                    ESP_LOGI(SPP_TAG, "Apagando configuração");
                    deleteFileConfig();
                    const char *response = "Configuração apagada\n";
                    spp_send_data(param->data_ind.handle, response);
                } else {
                    const char *response = "Fim da mensagem\n";
                    spp_send_data(param->data_ind.handle, response);
                    writeFileConfig(received_data);
                }
            } else {
                ESP_LOGE(SPP_TAG, "Received data too long to store");
            }
            break;
        case ESP_SPP_CONG_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
            break;
        case ESP_SPP_WRITE_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_WRITE_EVT");
            break;
        case ESP_SPP_SRV_OPEN_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT status:%d handle:%"PRIu32", rem_bda:[%s]",
                     param->srv_open.status, param->srv_open.handle, bda2str(param->srv_open.rem_bda, bda_str, sizeof(bda_str)));
            gettimeofday(&time_old, NULL);
            break;
        case ESP_SPP_SRV_STOP_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_STOP_EVT");
            break;
        case ESP_SPP_UNINIT_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_UNINIT_EVT");
            break;
        default:
            ESP_LOGI(SPP_TAG, "Unhandled event: %d", event);
            break;
    }
}

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    char bda_str[18] = {0};

    switch (event) {
        case ESP_BT_GAP_AUTH_CMPL_EVT:
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(SPP_TAG, "authentication success: %s bda:[%s]", param->auth_cmpl.device_name,
                         bda2str(param->auth_cmpl.bda, bda_str, sizeof(bda_str)));
            } else {
                ESP_LOGE(SPP_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
            }
            break;
        case ESP_BT_GAP_PIN_REQ_EVT:
            ESP_LOGI(SPP_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
            if (param->pin_req.min_16_digit) {
                ESP_LOGI(SPP_TAG, "Input pin code: 0000 0000 0000 0000");
                esp_bt_pin_code_t pin_code = {0};
                esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
            } else {
                ESP_LOGI(SPP_TAG, "Input pin code: 1234");
                esp_bt_pin_code_t pin_code;
                pin_code[0] = '1';
                pin_code[1] = '2';
                pin_code[2] = '3';
                pin_code[3] = '4';
                esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
            }
            break;

#if (CONFIG_EXAMPLE_SSP_ENABLED == true)
        case ESP_BT_GAP_CFM_REQ_EVT:
            ESP_LOGI(SPP_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %"PRIu32, param->cfm_req.num_val);
            esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
            break;
        case ESP_BT_GAP_KEY_NOTIF_EVT:
            ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%"PRIu32, param->key_notif.passkey);
            break;
        case ESP_BT_GAP_KEY_REQ_EVT:
            ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
            break;
#endif

        case ESP_BT_GAP_MODE_CHG_EVT:
            ESP_LOGI(SPP_TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d bda:[%s]", param->mode_chg.mode,
                     bda2str(param->mode_chg.bda, bda_str, sizeof(bda_str)));
            break;

        default:
            ESP_LOGI(SPP_TAG, "event: %d", event);
            break;
    }
}
