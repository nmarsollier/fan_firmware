/* Buttons — buttons.c — local push button.
 *
 * One push-to-GND button (active-low): the fan mode cycle. It is read on the
 * runtime loop, debounced, and its press edge triggers the action.
 */
#include "buttons.h"

#include "config.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "fan.h"

/* Samples a level change must persist before it is accepted. At the 200 ms
 * runtime-loop cadence contact bounce (a few ms) never spans a full sample, so
 * one sample is enough to reject glitches while keeping the press responsive. */
#define BUTTON_DEBOUNCE_SAMPLES 1

typedef struct {
    bool pressed;   /* current debounced level */
    uint8_t count;  /* consecutive samples differing from `pressed` */
} Button;

static Button s_fan_button;

/* Button bridges the pin to GND, so pressed == LOW. */
static bool read_raw(gpio_num_t pin) {
    return gpio_get_level(pin) == 0;
}

/* Advance the debouncer and report whether a press edge occurred. */
static bool debounce(Button *b, bool raw) {
    bool was_pressed = b->pressed;

    if (raw == b->pressed) {
        b->count = 0;
    } else if (++b->count >= BUTTON_DEBOUNCE_SAMPLES) {
        b->pressed = raw;
        b->count = 0;
    }

    return b->pressed && !was_pressed;
}

/* Next fan mode in the cycle off → auto → manual → off. */
static FanMode next_fan_mode(FanMode mode) {
    switch (mode) {
    case FAN_MODE_OFF:    return FAN_MODE_AUTO;
    case FAN_MODE_AUTO:   return FAN_MODE_MANUAL;
    case FAN_MODE_MANUAL: return FAN_MODE_OFF;
    default:              return FAN_MODE_AUTO;
    }
}

void buttons_init(void) {
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << BUTTON_FAN_MODE_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));
}

void buttons_update(void) {
    bool fan_press = debounce(&s_fan_button, read_raw(BUTTON_FAN_MODE_GPIO));

    if (fan_press) {
        fan_set_mode(next_fan_mode(fan_get_mode()));
    }
}

bool buttons_fan_mode_pressed(void) {
    return s_fan_button.pressed;
}
