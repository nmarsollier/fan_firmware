/* Buttons — buttons.c — local push button.
 *
 * One push-to-GND button (active-low): the fan mode cycle. The pin is sampled
 * on a fast esp_timer tick and debounced; the press edge is applied on the
 * runtime loop.
 */
#include "buttons.h"

#include "config.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "fan.h"

/* Sample period and the stability a level change must persist before it is
 * accepted: 5 samples × 5 ms = 25 ms of a stable level, enough to reject
 * contact bounce (a few ms) without making the press feel laggy. */
#define BUTTON_SAMPLE_PERIOD_US  5000
#define BUTTON_DEBOUNCE_SAMPLES  5

typedef struct {
    bool pressed;        /* current debounced level */
    bool press_pending;  /* press edge awaiting the runtime loop */
    uint8_t count;       /* consecutive samples differing from `pressed` */
} Button;

static Button s_fan_button;
static esp_timer_handle_t s_sample_timer;

/* Button bridges the pin to GND, so pressed == LOW. */
static bool read_raw(gpio_num_t pin) {
    return gpio_get_level(pin) == 0;
}

/* Advance the debouncer on one sample; report whether a press edge occurred. */
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

/* Fast periodic sample: debounce only — the action runs on the runtime loop so
 * fan state is never touched from the timer context. */
static void sample_timer_cb(void *arg) {
    (void) arg;
    if (debounce(&s_fan_button, read_raw(BUTTON_FAN_MODE_GPIO))) {
        s_fan_button.press_pending = true;
    }
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

    const esp_timer_create_args_t args = {
        .callback = sample_timer_cb,
        .name = "button_sample",
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &s_sample_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_sample_timer, BUTTON_SAMPLE_PERIOD_US));
}

void buttons_update(void) {
    if (s_fan_button.press_pending) {
        s_fan_button.press_pending = false;
        fan_set_mode(next_fan_mode(fan_get_mode()));
    }
}

bool buttons_fan_mode_pressed(void) {
    return s_fan_button.pressed;
}
