/* Led — led_update.c
 *
 * Purpose: periodic LED state decision — the single public entry point for all
 * LED state changes.
 *
 * Called from the runtime loop every ~200 ms. Inspects the fan mode to pick the
 * correct LED state:
 *
 *   1. Off    -> red (solid)
 *   2. Auto   -> blue (solid)
 *   3. Manual -> green (solid)
 *
 * No other module calls led_set_state() directly. This keeps all LED logic
 * cohesive in one place.
 */
#include "led_internal.h"

#include "fan.h"

void led_update(void) {
    switch (fan_get_mode()) {
    case FAN_MODE_OFF:    led_set_state(LED_STATE_OFF);    break;
    case FAN_MODE_AUTO:   led_set_state(LED_STATE_AUTO);   break;
    case FAN_MODE_MANUAL: led_set_state(LED_STATE_MANUAL); break;
    }
}
