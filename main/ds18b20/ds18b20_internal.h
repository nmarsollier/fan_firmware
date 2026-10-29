/* ds18b20_internal.h — internal API shared among the ds18b20 files. */

#pragma once

#include <stdint.h>

#include "config.h"
#include "onewire_bus.h"
#include "ds18b20.h"

/* ── DS18B20 timing ───────────────────────────────────────── */

/* 12-bit conversion takes up to 750 ms (datasheet). */
#define DS18B20_CONVERT_TIME_US  750000

/* ── DS18B20 commands ─────────────────────────────────────── */

#define DS18B20_CMD_CONVERT_TEMP       0x44
#define DS18B20_CMD_READ_SCRATCHPAD    0xBE

/* ── Sensor state ─────────────────────────────────────────── */

typedef struct {
    onewire_bus_handle_t bus;
    bool present;
    bool converting;
    int64_t convert_started_us;
    bool valid;
    float temperature_c;
} Ds18b20Sensor;

extern Ds18b20Sensor ds18b20_sensor;
