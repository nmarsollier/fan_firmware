#pragma once

#include <stdbool.h>

/*
 * Board power sense — detects the external 12 V rail through the ADC. When
 * the 12 V rail is absent (USB-only) the board is unpowered and the motors
 * refuse to move.
 *
 * The ADC is read only from board_update() (runtime loop); board_has_external_power()
 * returns the cached value so request handlers never block on the ADC.
 *
 * The status LEDs are owned by the led module.
 */

/* Initialize the ADC channel that senses the external 12 V rail. */
void board_init(void);

/* Refresh the cached power state (call from the runtime loop). */
void board_update(void);

/* True when the external 12 V rail is present (cached). */
bool board_has_external_power(void);
