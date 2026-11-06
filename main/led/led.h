#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Status LED — the onboard RGB indicator (WS2812 on GPIO 48).
 *
 * NORMAL — green (solid), the fan is healthy (off or auto mode).
 * MANUAL — yellow (solid), the fan is in manual mode.
 * ERROR  — green beacon (blink-blink-pause); reserved for a fault, no fault
 *          source is wired yet.
 *
 * All state decisions are made inside led_update(), the single public entry
 * point for LED state changes. No other module should call led_set_state()
 * directly.
 */
typedef enum {
    LED_STATE_NORMAL,
    LED_STATE_MANUAL,
    LED_STATE_ERROR
} LedState;

/* Initialise the WS2812 (RMT) driver and start in NORMAL (green). */
void led_init(void);

/* Periodic update, called every ~200 ms from the runtime loop. */
void led_update(void);
