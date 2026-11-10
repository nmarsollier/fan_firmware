/* Fan — fan.c — Noctua NF-A8 PWM + 12 V relay.
 *
 * Three modes: OFF, AUTO and MANUAL. Drives the fan speed input on
 * FAN_PWM_GPIO (25 kHz LEDC) and the relay on FAN_RELAY_GPIO. The LEDC output
 * is inverted in hardware because the MOSFET that drives the fan's
 * open-collector input inverts too: writing a duty of N % makes the fan see
 * N %.
 */
#include "fan.h"

#include <string.h>

#include "config.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "ds18b20.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "FAN";

#define FAN_LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE
#define FAN_LEDC_TIMER      LEDC_TIMER_2
#define FAN_LEDC_CHANNEL    LEDC_CHANNEL_2
#define FAN_PWM_FREQ_HZ     25000
#define FAN_PWM_DUTY_RES    LEDC_TIMER_8_BIT
#define FAN_PWM_MAX_DUTY    255
#define FAN_RELAY_SETTLE_MS 20

/* AUTO fallback: hold the last speed through a brief 1-wire outage, then drop
 * to the safety speed (FAN_SPEED_MIN) once the reading has been stale this
 * long — i.e. a persistent sensor failure. */
#define FAN_AUTO_INVALID_TIMEOUT_US  5000000

static FanMode s_mode = FAN_MODE_AUTO;
static uint8_t s_manual_speed = FAN_SPEED_MAX;
static uint8_t s_effective_speed = 0;
static int64_t s_last_valid_us = 0;

/* Map a user speed (1..FAN_SPEED_MAX) to a PWM duty percent. */
static uint8_t speed_to_pwm_pct(uint8_t speed) {
    if (speed <= 0) {
        return 0;
    }
    if (speed >= FAN_SPEED_MAX) {
        return FAN_MAX_PWM_PCT;
    }
    return (uint8_t)(FAN_MIN_PWM_PCT
        + (uint32_t)(FAN_MAX_PWM_PCT - FAN_MIN_PWM_PCT)
          * (speed - FAN_SPEED_MIN) / (FAN_SPEED_MAX - FAN_SPEED_MIN));
}

static void fan_apply_pwm(uint8_t speed) {
    uint8_t pwm_pct = speed_to_pwm_pct(speed);
    uint32_t duty = (uint32_t)pwm_pct * FAN_PWM_MAX_DUTY / 100;
    ledc_set_duty(FAN_LEDC_SPEED_MODE, FAN_LEDC_CHANNEL, duty);
    ledc_update_duty(FAN_LEDC_SPEED_MODE, FAN_LEDC_CHANNEL);
}

static void fan_apply_relay(bool on) {
    gpio_set_level(FAN_RELAY_GPIO, on ? FAN_RELAY_ACTIVE_LEVEL : !FAN_RELAY_ACTIVE_LEVEL);
}

/* Apply the current mode + speed to the hardware. */
static void fan_apply(void) {
    fan_apply_relay(s_mode != FAN_MODE_OFF);
    fan_apply_pwm(s_mode == FAN_MODE_OFF ? 0 : s_effective_speed);
}

void fan_init(void) {
    gpio_config_t relay = {
        .pin_bit_mask = 1ULL << FAN_RELAY_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&relay));

    ledc_timer_config_t timer = {
        .speed_mode = FAN_LEDC_SPEED_MODE,
        .duty_resolution = FAN_PWM_DUTY_RES,
        .timer_num = FAN_LEDC_TIMER,
        .freq_hz = FAN_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t chan = {
        .gpio_num = FAN_PWM_GPIO,
        .speed_mode = FAN_LEDC_SPEED_MODE,
        .channel = FAN_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = FAN_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = { .output_invert = 1 },
    };
    ESP_ERROR_CHECK(ledc_channel_config(&chan));

    /* Deterministic relay start: drive the relay OFF first and let the coil and
     * driver settle, then boot in AUTO (which closes it). This guarantees the
     * relay starts from a known de-energized state regardless of the power-on
     * sequence. */
    fan_apply_relay(false);
    vTaskDelay(pdMS_TO_TICKS(FAN_RELAY_SETTLE_MS));

    /* Boot in AUTO: seed the speed from the mirror temperature and apply. */
    fan_set_mode(FAN_MODE_AUTO);

    ESP_LOGI(TAG, "PWM GPIO %d @ 25 kHz, relay GPIO %d (active-%s)",
             (int)FAN_PWM_GPIO, (int)FAN_RELAY_GPIO,
             FAN_RELAY_ACTIVE_LEVEL ? "high" : "low");
}

/* Mirror temperature that drives the AUTO mode (absolute threshold). */
static float fan_auto_mirror(void) {
    return ds18b20_get_temperature_c();
}

/* Target AUTO speed for a mirror temperature, given the current speed (min or
 * max). The hysteresis band is symmetric: hold the current speed inside
 * [threshold - h, threshold + h], drop to min only below the lower edge and
 * rise to max only above the upper edge. Pass FAN_SPEED_MIN as `current` when
 * (re)entering AUTO so the mode switch uses the upper edge and never skips the
 * band. */
static uint8_t fan_auto_speed(float mirror, uint8_t current) {
    if (current == FAN_SPEED_MAX) {
        return (mirror <= FAN_AUTO_MIRROR_THRESHOLD_C - FAN_AUTO_HYSTERESIS_C)
               ? FAN_SPEED_MIN : FAN_SPEED_MAX;
    }
    return (mirror >= FAN_AUTO_MIRROR_THRESHOLD_C + FAN_AUTO_HYSTERESIS_C)
           ? FAN_SPEED_MAX : FAN_SPEED_MIN;
}

void fan_update(void) {
    if (s_mode != FAN_MODE_AUTO) {
        return;
    }

    /* No valid mirror reading: hold the last speed through a brief 1-wire
     * outage, then drop to the safety speed (min) once the outage becomes a
     * persistent failure. */
    if (!ds18b20_reading_valid()) {
        if (esp_timer_get_time() - s_last_valid_us >= FAN_AUTO_INVALID_TIMEOUT_US) {
            s_effective_speed = FAN_SPEED_MIN;
            fan_apply_pwm(s_effective_speed);
        }
        return;
    }
    s_last_valid_us = esp_timer_get_time();

    s_effective_speed = fan_auto_speed(fan_auto_mirror(), s_effective_speed);
    fan_apply_pwm(s_effective_speed);
}

void fan_set_mode(FanMode mode) {
    s_mode = mode;

    if (mode == FAN_MODE_OFF) {
        s_effective_speed = 0;
    } else if (mode == FAN_MODE_MANUAL) {
        s_effective_speed = s_manual_speed;
    } else {
        /* AUTO — re-seed from the current mirror temperature, applying the
         * hysteresis from the "min" side so a mode switch observes the same
         * band as steady-state fan_update() (never skips it). */
        s_effective_speed = ds18b20_reading_valid()
                            ? fan_auto_speed(fan_auto_mirror(), FAN_SPEED_MIN)
                            : FAN_SPEED_MIN;
    }

    fan_apply();
}

FanMode fan_get_mode(void) {
    return s_mode;
}

void fan_set_speed(uint8_t speed) {
    if (speed < FAN_SPEED_MIN) {
        speed = FAN_SPEED_MIN;
    } else if (speed > FAN_SPEED_MAX) {
        speed = FAN_SPEED_MAX;
    }
    s_manual_speed = speed;

    if (s_mode == FAN_MODE_MANUAL) {
        s_effective_speed = s_manual_speed;
        fan_apply_pwm(s_effective_speed);
    }
}

uint8_t fan_get_manual_speed(void) {
    return s_manual_speed;
}

uint8_t fan_get_effective_speed(void) {
    return s_effective_speed;
}

uint8_t fan_get_intensity_pct(void) {
    return speed_to_pwm_pct(s_effective_speed);
}

bool fan_get_power(void) {
    return s_mode != FAN_MODE_OFF;
}

int fan_get_rpm(void) {
    uint8_t pwm = fan_get_intensity_pct();
    if (pwm == 0) {
        return 0;  /* fan off / stopped */
    }
    /* Noctua NF-A8 PWM: 450 RPM at 20 % duty, 2200 RPM at 100 % duty, linear
     * in between. Theoretical until the tachometer is wired. */
    return FAN_RPM_MIN
        + (int)((uint32_t)(FAN_RPM_MAX - FAN_RPM_MIN) * (pwm - FAN_MIN_PWM_PCT))
          / (FAN_MAX_PWM_PCT - FAN_MIN_PWM_PCT);
}

const char *fan_mode_to_string(FanMode mode) {
    switch (mode) {
    case FAN_MODE_OFF:    return "off";
    case FAN_MODE_AUTO:   return "auto";
    case FAN_MODE_MANUAL: return "manual";
    default:              return "off";
    }
}

bool fan_mode_from_string(const char *s, FanMode *out) {
    if (strcmp(s, "off") == 0)    { *out = FAN_MODE_OFF;    return true; }
    if (strcmp(s, "auto") == 0)   { *out = FAN_MODE_AUTO;   return true; }
    if (strcmp(s, "manual") == 0) { *out = FAN_MODE_MANUAL; return true; }
    return false;
}
