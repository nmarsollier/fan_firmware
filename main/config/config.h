#pragma once

#include "driver/gpio.h"

/*
 * FAN hardware configuration.
 */

/* Fan controller model identifier — reported in the serial protocol
 * `capabilities`. */
#define DEVICE_NAME "FAN"

/* =========================================================================
 * Pin assignments
 * ========================================================================= */

/* Status LED — onboard RGB (WS2812), driven via RMT. GPIO 48 on the common
 * Super Mini (clone variants may use GPIO 21). */
#define LED_GPIO        GPIO_NUM_48

/* Primary-mirror temperature sensor (DS18B20) — 1-wire on a dedicated GPIO. */
#define DS18B20_GPIO    GPIO_NUM_7

/* Primary-mirror fan — Noctua NF-A8 PWM + 12 V relay.
 * PWM drives the fan speed input (25 kHz) through an open-drain N-MOSFET;
 * the relay switches the fan's 12 V supply. */
#define FAN_PWM_GPIO           GPIO_NUM_1
#define FAN_RELAY_GPIO         GPIO_NUM_8
#define FAN_RELAY_ACTIVE_LEVEL 1
#define FAN_SPEED_MAX         100    /* user speed at full duty (2200 RPM) */
#define FAN_SPEED_MIN         1      /* user speed at FAN_MIN_PWM_PCT (≈450 RPM) */
#define FAN_MIN_PWM_PCT       20     /* Noctua minimum duty (≈450 RPM) */
#define FAN_MAX_PWM_PCT       100    /* full duty (2200 RPM) */
#define FAN_RPM_MIN           450    /* NF-A8 PWM at FAN_MIN_PWM_PCT */
#define FAN_RPM_MAX           2200   /* NF-A8 PWM at FAN_MAX_PWM_PCT */

/* AUTO mode: mirror temperature above which the fan runs at max speed, with
 * hysteresis (max at T+h, min at T−h). Absolute threshold, to be calibrated. */
#define FAN_AUTO_MIRROR_THRESHOLD_C  20.0f
#define FAN_AUTO_HYSTERESIS_C         0.1f

/* =========================================================================
 * Local buttons — push-to-GND (active-low, internal pull-ups).
 * ========================================================================= */

/* Fan mode cycle (off → auto → manual → off). */
#define BUTTON_FAN_MODE_GPIO        GPIO_NUM_3
