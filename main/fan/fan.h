#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Fan subsystem — Noctua NF-A8 PWM with a 12 V relay.
 *
 * Three modes: OFF (relay off), AUTO (speed from the mirror temperature, an
 * absolute threshold) and MANUAL (speed set by the client). The speed input is
 * driven with a 25 kHz PWM through an open-drain N-MOSFET. RPM is read from the
 * tachometer (not wired yet).
 */

typedef enum {
    FAN_MODE_OFF = 0,
    FAN_MODE_AUTO,
    FAN_MODE_MANUAL,
} FanMode;

void fan_init(void);

/* Periodic update — drives the AUTO speed from the mirror temperature. */
void fan_update(void);

FanMode fan_get_mode(void);
void fan_set_mode(FanMode mode);

/* Manual speed (1..100); applied immediately in MANUAL, remembered otherwise. */
void fan_set_speed(uint8_t speed);
uint8_t fan_get_manual_speed(void);

/* Effective speed 0..100 (0 when off, computed in AUTO, manual in MANUAL). */
uint8_t fan_get_effective_speed(void);

/* PWM intensity 0..100 (0 when off, ~20..100 when running) — drives the LED. */
uint8_t fan_get_intensity_pct(void);

/* Relay state: on in every mode except OFF. */
bool fan_get_power(void);

/* Estimated fan RPM derived from the PWM duty (Noctua NF-A8 PWM published
 * 450–2200 RPM range): 0 when off, 450–2200 otherwise. Theoretical — the
 * tachometer is not wired yet. */
int fan_get_rpm(void);

/* "off" | "auto" | "manual". */
const char *fan_mode_to_string(FanMode mode);
/* Parse a mode string; returns false on an unknown value. */
bool fan_mode_from_string(const char *s, FanMode *out);
