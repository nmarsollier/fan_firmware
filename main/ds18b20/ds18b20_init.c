/* Ds18b20 — ds18b20_init.c
 *
 * Purpose: bring up the mirror-temperature sensor on its 1-wire bus.
 *
 * Creates an RMT-backed 1-wire bus on DS18B20_GPIO and probes for the sensor
 * with a reset pulse. A missing sensor is not fatal: the environment module
 * reports the mirror temperature unavailable.
 */
#include "ds18b20_internal.h"

#include "esp_log.h"

static const char *TAG = "DS18B20_INIT";

Ds18b20Sensor ds18b20_sensor;

esp_err_t ds18b20_init(void) {
    ds18b20_sensor.bus = NULL;
    ds18b20_sensor.present = false;
    ds18b20_sensor.converting = false;
    ds18b20_sensor.convert_started_us = 0;
    ds18b20_sensor.valid = false;
    ds18b20_sensor.temperature_c = 0.0f;

    onewire_bus_config_t bus_config = {
        .bus_gpio_num = DS18B20_GPIO,
        .flags.en_pull_up = true,
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 9,  /* 8 scratchpad bytes + CRC */
    };

    esp_err_t err = onewire_new_bus_rmt(&bus_config, &rmt_config, &ds18b20_sensor.bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "1-wire bus on GPIO %d failed: %s",
                 (int) DS18B20_GPIO, esp_err_to_name(err));
        return err;
    }

    /* A reset pulse finds any device on the bus. */
    err = onewire_bus_reset(ds18b20_sensor.bus);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "no DS18B20 on GPIO %d (%s) — mirror unavailable",
                 (int) DS18B20_GPIO, esp_err_to_name(err));
        return err;
    }

    ds18b20_sensor.present = true;
    ESP_LOGI(TAG, "DS18B20 ready on GPIO %d", (int) DS18B20_GPIO);
    return ESP_OK;
}
