#ifndef BT_UTILS_H
#define BT_UTILS_H

#include <stdint.h>
#include <stddef.h>
#include "esp_spp_api.h"
#include "esp_gap_bt_api.h"
#include "savefile.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const esp_spp_mode_t esp_spp_mode;
extern const bool esp_spp_enable_l2cap_ertm;


char *bda2str(uint8_t *bda, char *str, size_t size);
void trim_whitespace(char *str);
void spp_send_data(uint32_t handle, const char *data);
void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

#ifdef __cplusplus
}
#endif

#endif // BT_UTILS_H
