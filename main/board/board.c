/* Board — board.c
 *
 * Purpose: sense whether the board is powered from the 12 V rail or is
 * running on USB only. Drives the ERROR state of the motors (and, through the
 * led module, the board LED) when the 12 V rail is absent.
 */
#include "board.h"
#include "config.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "BOARD";

/*
 * Divider on the 12 V rail. Raw threshold ~2000 cleanly separates the
 * powered (~full scale) from the unpowered (~0) state.
 * TODO: confirm the divider ratio and tune this threshold.
 */
#define BOARD_POWER_PRESENT_THRESHOLD_RAW 2000

static adc_oneshot_unit_handle_t s_adc_handle;
static bool s_power_ok;

void board_init(void) {
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = BOARD_POWER_ADC_UNIT,
    };

    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit: %s", esp_err_to_name(err));
        return;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    err = adc_oneshot_config_channel(s_adc_handle, BOARD_POWER_ADC_CHANNEL, &chan_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel: %s", esp_err_to_name(err));
        return;
    }
}

void board_update(void) {
    if (s_adc_handle == NULL) {
        /* Sensor not initialised — fail safe (treat as unpowered). */
        s_power_ok = false;
        return;
    }

    int raw = 0;
    if (adc_oneshot_read(s_adc_handle, BOARD_POWER_ADC_CHANNEL, &raw) != ESP_OK) {
        /* Read failure — fail safe (treat as unpowered). */
        s_power_ok = false;
        return;
    }
    s_power_ok = raw > BOARD_POWER_PRESENT_THRESHOLD_RAW;
}

bool board_has_external_power(void) {
    return s_power_ok;
}
