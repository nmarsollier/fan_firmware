/* Led — led_init.c
 *
 * Purpose: initialise the RMT-based WS2812 driver on the onboard GPIO.
 *
 * Configures an RMT TX channel at 10 MHz with a bytes encoder tuned to the
 * WS2812 bit timing, and starts with the NORMAL (green) color.
 */
#include "led_internal.h"

#include "esp_err.h"
#include "esp_log.h"

rmt_channel_handle_t led_tx_chan;
rmt_encoder_handle_t led_encoder;
SemaphoreHandle_t led_mutex;

static const char *TAG = "LED_INIT";

void led_init(void) {
    rmt_tx_channel_config_t tx_config = {
        .gpio_num = LED_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_RMT_RES_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
        /* The LED and the DS18B20 1-wire channels (onewire_bus RMT) share one
         * RMT group interrupt; its priority is set by the first channel created
         * (this one). The 1-wire driver leaves the priority unspecified, so it
         * inherits this one. */
        .intr_priority = 3,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_config, &led_tx_chan));
    ESP_ERROR_CHECK(rmt_enable(led_tx_chan));

    rmt_bytes_encoder_config_t enc_config = {
        .bit0 = {
            .duration0 = LED_WS2812_T0H,
            .level0 = 1,
            .duration1 = LED_WS2812_T0L,
            .level1 = 0,
        },
        .bit1 = {
            .duration0 = LED_WS2812_T1H,
            .level0 = 1,
            .duration1 = LED_WS2812_T1L,
            .level1 = 0,
        },
        .flags = { .msb_first = true },
    };
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&enc_config, &led_encoder));

    led_mutex = xSemaphoreCreateMutex();
    if (led_mutex == NULL) {
        ESP_LOGE(TAG, "failed to create LED mutex");
    }

    /* Set the initial NORMAL color; led_set_state() skips it because the default
     * state is already NORMAL, so the WS2812 would otherwise never light. */
    led_set_color(LED_NORMAL_R, LED_NORMAL_G, LED_NORMAL_B);
}
