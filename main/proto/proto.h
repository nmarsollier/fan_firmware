#pragma once

/*
 * Fan protocol — public entry point.
 *
 * The protocol is a length-prefixed JSON message stream carried over the
 * USB CDC-ACM transport (proto=2, role="accessories", device_id="fan"). It
 * exposes the fan and the mirror temperature so a desktop app translates them
 * into ASCOM Alpaca and a UI. See README.md in this folder for the wire
 * contract.
 *
 * proto_init() must be called after usb_cdc_init(): it wires the CDC receive
 * stream to the frame parser.
 */
void proto_init(void);
