# Fan protocol (USB CDC-ACM)

Serial contract between the PC (Go gateway) and the fan controller's ESP32-S3. It
uses the unified transport and identifies itself through the `capabilities`
handshake (`device_type="accessories"`, `device_id="fan"`, `proto=2`).

## Transport and framing

- USB CDC-ACM (TinyUSB), a single serial port.
- Frame: `[u32 little-endian length][JSON UTF-8]`, up to 2048 bytes per body, no
  application CRC (USB is reliable).
- The receiver accepts partial reads and several frames together; on error it drops a
  byte and resynchronizes.

## Message envelope

Request:

```json
{"proto":2,"type":"control","id":42,"session":"g-7f2a","boot_id":"a1b2c3d4",
 "target":"fan","op":"speed","params":{"value":80}}
```

Response (`ok:true` + `data`, or `ok:false` + `error`):

```json
{"proto":2,"type":"control","id":42,"session":"g-7f2a","boot_id":"a1b2c3d4",
 "ok":true,"data":{"speed":80}}
{"proto":2,"type":"control","id":42,"session":"g-7f2a","boot_id":"a1b2c3d4",
 "ok":false,"error":{"code":"OUT_OF_RANGE","message":"fan speed must be 1..100","field":null}}
```

- `type`: `capabilities` | `state` | `config` | `control` | `action`.
- `target`: `device` | `fan` | `environment` | `buttons`.
- `action` (for `type=action`) or `op` (for control/config) names the operation.
- `params` is an object; `id` is a non-zero uint32 (0 is reserved for events, not yet
  implemented); `session`/`boot_id` are strings.

`capabilities` is the exception: it uses the unified identification header below
instead of the session/data envelope.

## Unified capabilities

```json
{"type":"capabilities","id":1,"ok":true,
 "boot_id":"0e86a327","proto":2,
 "device_type":"accessories","device_id":"fan",
 "name":"FAN","firmware_version":"0.1.0",
 "capabilities":{
   "fan":{"mode":["off","auto","manual"],"speed":true,"rpm":true},
   "sensors":{"mirror":"DS18B20"}}}
```

- `device_type`: `mount` | `accessories` — the discriminator.
- `device_id`, `name`, `firmware_version`, `proto`, `boot_id`: shared identity.

## Implemented catalog

| Operation | Status |
|---|---|
| `capabilities` | Unified handshake header + fan capabilities (real). |
| `state / device` | `power_external_present`, `health`, `fan_mode`, `reset_reason` (real). |
| `state / fan` | `mode`, `power`, `speed`, `manual_speed`, `rpm`, `power_available` (real; `rpm` is a theoretical estimate from the PWM duty until the tach is wired). |
| `state / environment` | Mirror temperature only: `mirror_c` (real); ambient/humidity/dew are always `null`. |
| `state / buttons` | Fan button press state (real, debounced). |
| `config / <group> / get` | Read-only mock; `set` → `NOT_SUPPORTED`. |
| `control / fan / mode` | `params.value` (`off`/`auto`/`manual`). |
| `control / fan / speed` | `params.value` (`FAN_SPEED_MIN`–`FAN_SPEED_MAX`) → manual PWM (applied in MANUAL). |
| `action / device / restart` | Deferred reboot (real). |
| `control / device / session_open`, `heartbeat` | Minimal, no real lease enforcement. |

## Error codes (strings)

`INVALID_ARGUMENT`, `OUT_OF_RANGE`, `INTERNAL`, `NOT_SUPPORTED`.

## State error model

Every `state/<target>` response carries `ok:true` and a `data.error` field that
reports the subsystem's own condition:

- `null` — healthy.
- `"SENSOR_ABSENT"` — the DS18B20 was not detected at boot (or no valid reading yet);
  the `mirror_c` field is `null`.

A `state` request **always answers**: a sensor fault is returned as an error state,
never as a hang or a fabricated reading (no 0 °C placeholder).

## Pending (out of this iteration)

Events/subscription and push; real lease/heartbeat; `config/set` with persistence;
fan RPM (tach), real RPM readback.
