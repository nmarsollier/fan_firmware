/* Led — led_init.c
 *
 * Purpose: initialise every status LED's LEDC PWM channel. Each LED owns its
 * own timer (board = timer 0, fan = timer 3; timer 2 is the fan PWM).
 */
#include "led_internal.h"

#include "esp_err.h"

LedConfig led_configs[LED_COUNT] = {
    [LED_ID_BOARD]   = { .timer = LEDC_TIMER_0, .channel = LEDC_CHANNEL_0, .gpio = BOARD_LED_GPIO },
    [LED_ID_FAN]     = { .timer = LEDC_TIMER_3, .channel = LEDC_CHANNEL_3, .gpio = FAN_LED_GPIO },
};

LedState led_states[LED_COUNT] = {
    [LED_ID_BOARD]   = LED_STATE_DIM,
    [LED_ID_FAN]     = LED_STATE_DIM,
};

void led_init(void) {
    for (int i = 0; i < LED_COUNT; i++) {
        const LedConfig *cfg = &led_configs[i];

        ledc_timer_config_t timer_conf = {
            .speed_mode      = LED_MODE,
            .duty_resolution = LED_DUTY_RES,
            .timer_num       = cfg->timer,
            .freq_hz         = LED_FREQ_HZ,
            .clk_cfg         = LEDC_AUTO_CLK,
            .deconfigure     = false,
        };
        ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

        ledc_channel_config_t chan_conf = {
            .gpio_num       = cfg->gpio,
            .speed_mode     = LED_MODE,
            .channel        = cfg->channel,
            .intr_type      = LEDC_INTR_DISABLE,
            .timer_sel      = cfg->timer,
            .duty           = LED_DIM_DUTY,
            .hpoint         = 0,
            .sleep_mode     = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
            .flags          = { .output_invert = 0 },
        };
        ESP_ERROR_CHECK(ledc_channel_config(&chan_conf));
    }

    /* Fade service (needed for ledc_set_fade_time_and_start). */
    ESP_ERROR_CHECK(ledc_fade_func_install(0));
}
