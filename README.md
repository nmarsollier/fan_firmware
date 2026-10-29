# fan_firmware — Primary-mirror fan controller

Controls the primary-mirror cooling fan of a Newtonian telescope. This board drives
the fan and reads the mirror temperature; the focuser and the ambient/vibration
sensors live in their own boards (`eaf_firmware` and `temp_firmware`).

## Functionality

- Primary-mirror fan (Noctua NF-A8 PWM + 12 V relay), modes `off`/`auto`/`manual`.
- Mirror temperature (DS18B20) — the feedback signal for the AUTO mode.
- External 12 V detection (ADC) and a board status LED that flags a missing rail.
- A fan status LED (brightness follows fan intensity).
- A local fan-mode cycle button (off → auto → manual → off).

## Implementation status (October 2026)

| Module | Role | Pins |
|---|---|---|
| `board` | 12 V power sense (ADC). 12 V missing → board LED in *beacon* (error). | ADC `BOARD_POWER_ADC_CHANNEL` |
| `led` | Status LEDs (board + fan). | board `BOARD_LED_GPIO`, fan `FAN_LED_GPIO` |
| `ds18b20` | DS18B20 mirror temperature over 1-wire (driver: init, probe, non-blocking read). | `DS18B20_GPIO` |
| `fan` | Noctua NF-A8 PWM (25 kHz) + relay; modes `off`/`auto`/`manual`. | PWM `FAN_PWM_GPIO`, relay `FAN_RELAY_GPIO` |
| `buttons` | One local push button (active-low, debounced): fan mode cycle. | fan `BUTTON_FAN_MODE_GPIO` |
| `usb_cdc` | USB CDC-ACM transport (TinyUSB). | — |
| `proto` | `proto=2`, `device_id="fan"`; handlers `capabilities`/`state`/`action`/`config`/`control`. | — |

The protocol contract is documented in [`main/proto/README.md`](main/proto/README.md).

## Hardware

| Component | Part |
|---|---|
| Microcontroller | ESP32-S3 <https://www.amazon.com/dp/B0DG8L7MQ9> |
| Fan | Noctua NF-A8 PWM 12 V <https://www.amazon.com/dp/B00VG9PJBO> |
| Mirror temp sensor | DS18B20 <https://www.amazon.com/dp/B0FKG3HT9Q> |
| Step-down | LM2596 <https://www.amazon.com/gp/aw/d/B0GZ524QTV> |
| Relay | 12 V relay module <https://www.amazon.com/gp/aw/d/B0F3D1X27Y> |

## Electrical architecture

- The controller is fed with 12 V for the fan (and the ESP via a step-down).
- The fan has a relay that cuts only the fan's 12 V supply.
- The ESP32-S3 is powered two ways: USB from a PC, or 5 V from the 12 V rail via a step-down.
- 12 V monitor: determines whether the whole circuit is correctly fed with 12 V.

### ADC circuit

When there is no voltage, the circuit enters an error state: the board LED pulses,
marking the lack of 12 V supply.

### LEDs

Two LEDs: board (general state) and fan (speed). The circuit is the same for both:
LED cathode to GND, anode to a GPIO through a 100 Ω resistor; brightness is dimmed
via PWM duty.

- Board LED — beacon (error) when 12 V is missing; dim otherwise.
- Fan LED — brightness proportional to fan intensity (≈`FAN_MIN_PWM_PCT`–`FAN_MAX_PWM_PCT` %
  when running); off when unpowered or 12 V absent.

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
- `manual` mode uses the speed set by the client; `auto` computes it from the mirror
  temperature: `FAN_SPEED_MAX` above `FAN_AUTO_MIRROR_THRESHOLD_C` + hysteresis,
  `FAN_SPEED_MIN` below it (and while no valid reading is available). The threshold
  and hysteresis are tunable in `config.h`.
- A status LED mirrors the fan intensity (≈`FAN_MIN_PWM_PCT`–`FAN_MAX_PWM_PCT` % when running).

## Mirror temperature sensor — DS18B20

- Placed on the back face of the primary mirror to measure the glass temperature.
- Read over 1-wire on GPIO 6 (RMT-backed; internal pull-up enabled, a 4.7 kΩ
  pull-up to 3.3 V is recommended). The read is non-blocking: a 12-bit
  conversion (~750 ms) runs between runtime-loop ticks.
- It drives the fan's AUTO mode and is exposed to the gateway via `state/environment`
  (`mirror_c`).

## Communication

USB-CDC with JSON messages. Provides the fan state and mirror temperature, and
processes the fan commands needed for Alpaca. The Go gateway (`mount_panel`)
implements the Alpaca Switch V3 (and the mirror-temperature reading) on top of this
link.

## Alpaca

The Go interface (`mount_panel`) exposes the fan power/speed/RPM and the mirror
temperature through the Alpaca Switch V3 interface.
