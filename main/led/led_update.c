/* Led — led_update.c
 *
 * Purpose: periodic LED state decision — the single public entry point for
 * all LED state changes.
 *
 * The board LED reflects the external power rail; the fan LED brightness
 * follows the fan intensity. No other module calls led_set_state() directly.
 */
#include "led_internal.h"

#include "board.h"
#include "fan.h"

void led_update(void) {
    /* Board LED: dim when 12 V is present, beacon (error) when it is missing. */
    if (board_has_external_power()) {
        led_set_state(LED_ID_BOARD, LED_STATE_DIM);
    } else {
        led_set_state(LED_ID_BOARD, LED_STATE_BEACON);
    }

    /* Fan LED: brightness follows fan intensity, but only when 12 V is present
     * (the fan cannot run without it). */
    uint32_t fan_duty = 0;
    if (board_has_external_power() && fan_get_power()) {
        fan_duty = (uint32_t)fan_get_intensity_pct() * LED_BRIGHT_DUTY / 100;
    }
    led_set_duty(LED_ID_FAN, fan_duty);
}
