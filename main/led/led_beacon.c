/* Led — led_beacon.c
 *
 * Purpose: beacon animation for the error state, one timer per LED.
 *
 * Two quick blinks, then a slow 1 s fade-out — like a lighthouse — so an
 * error is obvious at a glance. Driven by a one-shot esp_timer per LED that
 * re-arms itself for each phase.
 */
#include "led_internal.h"

#include <stdint.h>

#include "esp_timer.h"

/* One step of the beacon cycle. */
typedef struct {
    uint32_t duty;      /* target duty (13-bit) */
    uint32_t hold_ms;   /* how long this step lasts */
    uint32_t fade_ms;   /* fade duration to reach `duty`; 0 = instant */
} BeaconStep;

/* blink -> blink -> slow 1 s fade-out -> repeat */
static const BeaconStep s_beacon_steps[] = {
    { LED_BRIGHT_DUTY, 200, 0   },  /* blink 1 on  */
    { 0,               100, 0   },  /* blink 1 off */
    { LED_BRIGHT_DUTY, 200, 0   },  /* blink 2 on  */
    { 0,               1000, 800 }, /* slow fade out */
};
#define BEACON_STEP_COUNT (sizeof(s_beacon_steps) / sizeof(s_beacon_steps[0]))

static esp_timer_handle_t s_beacon_timers[LED_COUNT];
static unsigned int       s_beacon_phases[LED_COUNT];

static void beacon_tick(void *arg) {
    LedId id = (LedId)(uintptr_t) arg;
    const LedConfig *cfg = &led_configs[id];
    const BeaconStep *step = &s_beacon_steps[s_beacon_phases[id]];

    if (step->fade_ms > 0) {
        ledc_set_fade_time_and_start(LED_MODE, cfg->channel, step->duty,
                                     step->fade_ms, LEDC_FADE_NO_WAIT);
    } else {
        ledc_set_duty(LED_MODE, cfg->channel, step->duty);
        ledc_update_duty(LED_MODE, cfg->channel);
    }

    s_beacon_phases[id] = (s_beacon_phases[id] + 1) % BEACON_STEP_COUNT;
    esp_timer_start_once(s_beacon_timers[id], (uint64_t) step->hold_ms * 1000UL);
}

void led_beacon_start(LedId id) {
    if (s_beacon_timers[id]) {
        return;
    }

    s_beacon_phases[id] = 0;

    esp_timer_create_args_t args = {
        .callback = beacon_tick,
        .arg      = (void *) (uintptr_t) id,
        .name     = "led_beacon",
    };
    esp_timer_create(&args, &s_beacon_timers[id]);

    /* Apply the first step immediately and schedule the next. */
    beacon_tick((void *) (uintptr_t) id);
}

void led_beacon_stop(LedId id) {
    if (!s_beacon_timers[id]) {
        return;
    }
    esp_timer_stop(s_beacon_timers[id]);
    esp_timer_delete(s_beacon_timers[id]);
    s_beacon_timers[id] = NULL;
}
