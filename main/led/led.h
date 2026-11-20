#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Status LED — the onboard RGB indicator (WS2812 on GPIO 48).
 *
 * OFF    — red (solid), the fan is off.
 * AUTO   — blue (solid), the fan is in auto mode.
 * MANUAL — green (solid), the fan is in manual mode.
 *
 * All state decisions are made inside led_update(), the single public entry
 * point for LED state changes. No other module should call led_set_state()
 * directly.
 */
typedef enum {
    LED_STATE_OFF,
    LED_STATE_AUTO,
    LED_STATE_MANUAL
} LedState;

/* Initialise the WS2812 (RMT) driver and start in AUTO (blue). */
void led_init(void);

/* Periodic update, called every ~200 ms from the runtime loop. */
void led_update(void);
