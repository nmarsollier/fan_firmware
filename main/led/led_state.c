/* Led — led_state.c
 *
 * Purpose: apply LED state transitions (dim / bright / beacon).
 *
 * Internal helper called exclusively from led_update(); other modules must
 * never call led_set_state() directly.
 */
#include "led_internal.h"

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "LED_STATE";

static void led_start_fade(LedId id, uint32_t target_duty, uint32_t time_ms) {
    const LedConfig *cfg = &led_configs[id];
    esp_err_t err = ledc_set_fade_time_and_start(
        LED_MODE, cfg->channel, target_duty, time_ms, LEDC_FADE_NO_WAIT);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "fade start failed: %s", esp_err_to_name(err));
    }
}

void led_set_state(LedId id, LedState state) {
    if (state == led_states[id]) {
        return;
    }
    led_states[id] = state;

    /* Stop the animation (if any) before applying the new state. */
    led_beacon_stop(id);

    switch (state) {
    case LED_STATE_DIM:
        led_start_fade(id, LED_DIM_DUTY, LED_FADE_MS);
        break;
    case LED_STATE_BRIGHT:
        led_start_fade(id, LED_BRIGHT_DUTY, LED_FADE_MS);
        break;
    case LED_STATE_BEACON:
        led_beacon_start(id);
        break;
    case LED_STATE_OFF:
        led_start_fade(id, 0, LED_FADE_MS);
        break;
    }
}

void led_set_duty(LedId id, uint32_t duty) {
    const LedConfig *cfg = &led_configs[id];
    ledc_set_duty(LED_MODE, cfg->channel, duty);
    ledc_update_duty(LED_MODE, cfg->channel);
}
