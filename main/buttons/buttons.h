#pragma once

#include <stdbool.h>

/*
 * Local buttons — one push button, push-to-GND (active-low, internal pull-up):
 *
 *   1. fan mode cycle: off → auto → manual → off (BUTTON_FAN_MODE_GPIO).
 *
 * The pin is sampled and debounced on a fast timer tick started by
 * buttons_init(); buttons_update() applies the press edge. The *_pressed()
 * getter reports the current debounced level, for the protocol `state/buttons`
 * handler.
 */

void buttons_init(void);

/* Apply any pending button press. Called from the runtime loop. */
void buttons_update(void);

bool buttons_fan_mode_pressed(void);
