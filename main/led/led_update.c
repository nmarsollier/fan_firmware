/* Led — led_update.c
 *
 * Purpose: periodic LED state decision — the single public entry point for all
 * LED state changes.
 *
 * Called from the runtime loop every ~200 ms. Inspects the mirror sensor and
 * the fan mode to pick the correct LED state:
 *
 *   1. Mirror sensor absent -> green beacon (blink-blink-pause)
 *   2. Manual mode          -> yellow (solid)
 *   3. Off / auto           -> green (normal)
 *
 * No other module calls led_set_state() directly. This keeps all LED logic
 * cohesive in one place.
 */
#include "led_internal.h"

#include "ds18b20.h"
#include "fan.h"

void led_update(void) {
    if (!ds18b20_is_present()) {
        led_set_state(LED_STATE_ERROR);
    } else if (fan_get_mode() == FAN_MODE_MANUAL) {
        led_set_state(LED_STATE_MANUAL);
    } else {
        led_set_state(LED_STATE_NORMAL);
    }
}
