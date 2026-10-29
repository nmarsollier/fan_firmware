/* led_internal.h — internal API shared among the led module files. */

#pragma once

#include <stdint.h>

#include "led.h"
#include "config.h"
#include "driver/ledc.h"

/* ── LEDC hardware configuration ──────────────────────────── */

#define LED_MODE          LEDC_LOW_SPEED_MODE
#define LED_FREQ_HZ       5000
#define LED_DUTY_RES      LEDC_TIMER_13_BIT

/* ── Brightness levels (13-bit range: 0 – 8191) ──────────── */

#define LED_DIM_DUTY      1000   /* idle, ~12 % */
#define LED_BRIGHT_DUTY   8191   /* 100 % */

/* ── Timing ───────────────────────────────────────────────── */

#define LED_FADE_MS       1000

/* ── LED ids and states ───────────────────────────────────── */

typedef enum {
    LED_ID_BOARD = 0,
    LED_ID_FAN,
    LED_COUNT
} LedId;

typedef enum {
    LED_STATE_DIM,
    LED_STATE_BRIGHT,
    LED_STATE_BEACON,
    LED_STATE_OFF,
} LedState;

/* Per-LED hardware configuration (defined in led_init.c). */
typedef struct {
    ledc_timer_t   timer;
    ledc_channel_t channel;
    gpio_num_t     gpio;
} LedConfig;

extern LedConfig led_configs[LED_COUNT];
extern LedState  led_states[LED_COUNT];

/* ── Internal helpers ─────────────────────────────────────── */

void led_set_state(LedId id, LedState state);
void led_beacon_start(LedId id);
void led_beacon_stop(LedId id);

/* Set a LED to a raw duty (0..LED_BRIGHT_DUTY) immediately, bypassing the
 * state machine. Used for the fan LED, whose brightness tracks fan speed. */
void led_set_duty(LedId id, uint32_t duty);
