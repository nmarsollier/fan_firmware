# fan_firmware — Primary-mirror fan controller

Controls the primary-mirror cooling fan of a Newtonian telescope. This board drives
the fan; the focuser and the ambient/vibration sensors live in their own boards
(`eaf_firmware` and `amb_firmware`).

## Stella Lyra variant

This branch is a special build of the controller for the **Stella Lyra**
Newtonian. It differs from the generic fan controller in two ways:

- **No mirror temperature control** — the DS18B20 sensor (and its
  `state/environment` protocol endpoint) is not present in this build.
- **Autonomous by default** — the board runs standalone most of the time: the fan
  mode is cycled with the local button and the AUTO speed schedule is purely
  time-based. It is normally disconnected from `mount_panel`; the USB port is
  used mainly for flashing. When a host is attached, the JSON protocol still
  works unchanged.

## Functionality

- Primary-mirror fan (Noctua NF-A8 PWM + 12 V relay), modes `off`/`auto`/`manual`.
- An onboard RGB status LED (red = off, blue = auto, green = manual).
- A local fan-mode cycle button (off → auto → manual → off).
- A single USB connector for both the app (CDC-ACM) and flashing (ROM download).

## Implementation status (November 2026)

| Module | Role | Pins |
|---|---|---|
| `led` | Onboard RGB status LED (WS2812 via RMT): red off, blue auto, green manual. | `LED_GPIO` (GPIO48) |
| `fan` | Noctua NF-A8 PWM (25 kHz) + relay; modes `off`/`auto`/`manual`. | PWM `FAN_PWM_GPIO` (GPIO1), relay `FAN_RELAY_GPIO` (GPIO8) |
| `buttons` | One local push button (active-low, debounced): fan mode cycle. | `BUTTON_FAN_MODE_GPIO` (GPIO3) |
| `usb_cdc` | USB CDC-ACM transport (TinyUSB) + download-mode handoff for flashing. | — |
| `proto` | `proto=2`, `device_id="fan"`; handlers `capabilities`/`state`/`action`/`config`/`control`. | — |

The protocol contract is documented in [`main/proto/README.md`](main/proto/README.md).

## Hardware

| Component | Part |
|---|---|
| Microcontroller | ESP32-S3 Super Mini <https://www.amazon.com/dp/B0GY3M83NL> |
| Fan | Noctua NF-A8 PWM 12 V <https://www.amazon.com/dp/B00VG9PJBO> |
| Step-down | LM2596 <https://www.amazon.com/gp/aw/d/B0GZ524QTV> |
| Relay | 12 V relay module <https://www.amazon.com/gp/aw/d/B0F3D1X27Y> |

## Electrical architecture

- The controller is fed with 12 V for the fan (and the ESP via a step-down).
- The fan has a relay that cuts only the fan's 12 V supply.
- The ESP32-S3 is powered two ways: USB from a PC, or 5 V from the 12 V rail via a step-down.

### Status LED

A single onboard RGB WS2812 (GPIO48), driven via RMT:

- Red — fan in `off` mode.
- Blue — fan in `auto` mode.
- Green — fan in `manual` mode.

## Noctua NF-A8 PWM fan

Three modes: `off`, `auto` and `manual`. The button cycles them (off → auto →
manual → off); the mode is also settable from the protocol.

- The fan's 12 V supply is switched by a relay on a dedicated GPIO (off in `off` mode).
- Speed is set with PWM on a GPIO at 25 kHz. The fan's PWM input has an internal
  5 V pull-up, so the 3.3 V GPIO drives it through an open-drain N-MOSFET (2N7002).
- Speed range: `FAN_SPEED_MIN`–`FAN_SPEED_MAX`, mapped linearly to `FAN_MIN_PWM_PCT`–`FAN_MAX_PWM_PCT` % PWM (≈450–2200 RPM); 0 % stops it.
- The protocol reports a **theoretical RPM** (`state/fan` → `rpm`) derived from the
  PWM duty and Noctua's published 450–2200 RPM curve (0 when off). It is an
  estimate until the tachometer is wired.
- `manual` mode uses the speed set by the client; `auto` runs `FAN_SPEED_MAX` for
  the first `FAN_AUTO_HIGH_DURATION_US` after power-on, then `FAN_SPEED_MIN`.

## Communication

USB-CDC with JSON messages. Provides the fan state and processes the fan commands
needed for Alpaca. The Go gateway (`mount_panel`) implements the Alpaca Switch V3 on
top of this link.

## Building and flashing

The board has a single USB-C connector: the app enumerates as a CDC-ACM serial
port, and flashing uses the same port after the firmware reboots into ROM download
mode — no BOOT button needed.

```sh
make flash            # build + button-less flash (requests download mode first)
make flash-download   # flash a board already in ROM download mode (first flash)
make enter-flash      # only enter download mode
```

`flash` sends the protocol's `action/device/bootloader` command, waits for the
download port, then runs `idf.py flash`. `flash-download` targets a blank board
that has no app to answer the command.

## Alpaca

The Go interface (`mount_panel`) exposes the fan power/speed/RPM through the Alpaca
Switch V3 interface.
