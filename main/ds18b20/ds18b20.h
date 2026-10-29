#pragma once

#include <stdbool.h>

#include "esp_err.h"

/*
 * DS18B20 mirror-temperature sensor on a dedicated 1-wire GPIO (DS18B20_GPIO).
 *
 * The sensor is read with a non-blocking state machine: each ds18b20_update()
 * advances a 12-bit conversion (~750 ms) so the runtime loop never stalls on
 * the slow conversion. The latest successful reading is cached and exposed by
 * the getters below.
 */

/*
 * Bring up the 1-wire bus and probe for the sensor. Returns ESP_OK when the
 * sensor answered the boot reset, or an error otherwise. A missing sensor is
 * not fatal: the environment module just reports the mirror unavailable.
 */
esp_err_t ds18b20_init(void);

/* True once the sensor answered the boot reset. */
bool ds18b20_is_present(void);

/*
 * Advance the non-blocking conversion/read state machine. Called from the
 * runtime loop; safe to call every tick.
 */
void ds18b20_update(void);

/* True once a temperature has been read successfully. */
bool ds18b20_reading_valid(void);

/* Last cached mirror temperature in °C (valid only when reading_valid()). */
float ds18b20_get_temperature_c(void);
