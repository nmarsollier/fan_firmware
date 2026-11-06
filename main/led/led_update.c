/* Led — led_update.c
 *
 * Purpose: periodic LED state decision — the single public entry point for all
 * LED state changes.
 *
 * Called from the runtime loop every ~200 ms. Inspects the fan mode to pick the
 * correct LED state:
 *
 *   1. Manual mode   -> yellow (solid)
 *   2. Off / auto    -> green (normal)
 *
 * No other module calls led_set_state() directly. This keeps all LED logic
 * cohesive in one place.
 */
#include "led_internal.h"

#include "fan.h"

void led_update(void) {
    if (fan_get_mode() == FAN_MODE_MANUAL) {
        led_set_state(LED_STATE_MANUAL);
    } else {
        led_set_state(LED_STATE_NORMAL);
    }
}
