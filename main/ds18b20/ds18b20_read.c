/* Ds18b20 — ds18b20_read.c
 *
 * Purpose: read the mirror temperature without stalling the runtime loop.
 *
 * The DS18B20 converts in ~750 ms. Instead of blocking, ds18b20_update()
 * starts a conversion on one tick and reads the scratchpad on a later tick
 * once the conversion has finished. The temperature is validated with the
 * scratchpad CRC-8 and cached.
 */
#include "ds18b20_internal.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "onewire_cmd.h"
#include "onewire_crc.h"

static const char *TAG = "DS18B20_READ";

bool ds18b20_is_present(void) {
    return ds18b20_sensor.present;
}

bool ds18b20_reading_valid(void) {
    return ds18b20_sensor.valid;
}

float ds18b20_get_temperature_c(void) {
    return ds18b20_sensor.temperature_c;
}

/* Start a conversion (reset + skip-rom + convert). Non-blocking. */
static esp_err_t start_conversion(void) {
    esp_err_t err = onewire_bus_reset(ds18b20_sensor.bus);
    if (err != ESP_OK) {
        return err;
    }

    uint8_t cmd[2] = { ONEWIRE_CMD_SKIP_ROM, DS18B20_CMD_CONVERT_TEMP };
    return onewire_bus_write_bytes(ds18b20_sensor.bus, cmd, sizeof(cmd));
}

/* Read and validate the 9-byte scratchpad, then convert to °C. */
static esp_err_t read_scratchpad(float *temperature_c) {
    esp_err_t err = onewire_bus_reset(ds18b20_sensor.bus);
    if (err != ESP_OK) {
        return err;
    }

    uint8_t cmd[2] = { ONEWIRE_CMD_SKIP_ROM, DS18B20_CMD_READ_SCRATCHPAD };
    err = onewire_bus_write_bytes(ds18b20_sensor.bus, cmd, sizeof(cmd));
    if (err != ESP_OK) {
        return err;
    }

    uint8_t sp[9] = { 0 };
    err = onewire_bus_read_bytes(ds18b20_sensor.bus, sp, sizeof(sp));
    if (err != ESP_OK) {
        return err;
    }

    /* Scratchpad CRC covers the first 8 bytes. */
    if (onewire_crc8(0, sp, 8) != sp[8]) {
        return ESP_ERR_INVALID_CRC;
    }

    /* Reject the power-on default (85.0 °C) of an unread sensor. */
    if (sp[6] == 0x0C && sp[1] == 0x05 && sp[0] == 0x50) {
        ESP_LOGW(TAG, "scratchpad holds the power-on value (85.0)");
        return ESP_ERR_INVALID_STATE;
    }

    /* Mask the unused low bits for the sensor's resolution (12-bit default). */
    const uint8_t lsb_mask[4] = { 0x07, 0x03, 0x01, 0x00 };
    uint8_t lsb = sp[0] & ~lsb_mask[sp[4] >> 5];
    int16_t raw = (int16_t)((sp[1] << 8) | lsb);
    *temperature_c = raw / 16.0f;

    return ESP_OK;
}

void ds18b20_update(void) {
    if (!ds18b20_sensor.present) {
        return;
    }

    if (!ds18b20_sensor.converting) {
        if (start_conversion() != ESP_OK) {
            /* Sensor went away mid-run; drop the last reading and retry next tick. */
            ds18b20_sensor.valid = false;
            return;
        }
        ds18b20_sensor.converting = true;
        ds18b20_sensor.convert_started_us = esp_timer_get_time();
        return;
    }

    /* Wait for the conversion to finish before reading. */
    if (esp_timer_get_time() - ds18b20_sensor.convert_started_us < DS18B20_CONVERT_TIME_US) {
        return;
    }

    float t;
    if (read_scratchpad(&t) == ESP_OK) {
        ds18b20_sensor.temperature_c = t;
        ds18b20_sensor.valid = true;
    } else {
        ds18b20_sensor.valid = false;
    }
    ds18b20_sensor.converting = false;
}
