/* Proto — proto_state.c — STATE: coherent snapshots per target.
 *
 * Routed by `target`: device, fan, environment (mirror only), buttons.
 */
#include "proto_internal.h"

#include <string.h>

#include "esp_system.h"

#include "buttons.h"
#include "ds18b20.h"

static void get_session(const char *req, char *out, size_t cap) {
    proto_json_get_string(req, "session", out, cap);
}

static const char *reset_reason_str(void) {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON: return "power_on";
        case ESP_RST_SW:      return "software";
        case ESP_RST_PANIC:   return "panic";
        case ESP_RST_BROWNOUT:return "brownout";
        case ESP_RST_WDT:
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:return "watchdog";
        case ESP_RST_USB:     return "usb";
        default:              return "unknown";
    }
}

/* ── state / device ────────────────────────────────────────── */

static void state_device(char *out, size_t cap, uint32_t id, const char *session) {
    bool power = board_has_external_power();

    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "state", id, session, true);
    proto_buf_append(out, cap, &len,
        ",\"data\":{\"power_external_present\":%s,\"health\":\"ok\","
        "\"fan_mode\":\"%s\",\"reset_reason\":\"%s\",\"faults\":[]}}",
        power ? "true" : "false",
        fan_mode_to_string(fan_get_mode()),
        reset_reason_str());
}

/* ── state / fan ───────────────────────────────────────────── */

static void state_fan(char *out, size_t cap, uint32_t id, const char *session) {
    const char *mode = fan_mode_to_string(fan_get_mode());
    bool power = fan_get_power();
    int speed = fan_get_effective_speed();
    int manual = fan_get_manual_speed();
    int rpm = fan_get_rpm();

    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "state", id, session, true);
    proto_buf_append(out, cap, &len,
        ",\"data\":{\"error\":null,\"mode\":\"%s\",\"power\":%s,\"speed\":%d,"
        "\"manual_speed\":%d,\"power_available\":%s,\"rpm\":",
        mode, power ? "true" : "false", speed, manual,
        board_has_external_power() ? "true" : "false");
    if (rpm >= 0) {
        proto_buf_append(out, cap, &len, "%d}}", rpm);
    } else {
        proto_buf_append(out, cap, &len, "null}}");
    }
}

/* ── state / environment (mirror only) ─────────────────────── */

static void state_environment(char *out, size_t cap, uint32_t id, const char *session) {
    bool mirror_valid = ds18b20_reading_valid();

    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "state", id, session, true);

    if (!mirror_valid) {
        proto_buf_append(out, cap, &len,
            ",\"data\":{\"error\":\"SENSOR_ABSENT\",\"ambient_c\":null,"
            "\"humidity_pct\":null,\"mirror_c\":null,\"dew_point_c\":null,"
            "\"mirror_ambient_delta_c\":null,\"mirror_dew_margin_c\":null}}");
        return;
    }

    proto_buf_append(out, cap, &len,
        ",\"data\":{\"error\":null,\"ambient_c\":null,\"humidity_pct\":null,"
        "\"mirror_c\":%.2f,\"dew_point_c\":null,"
        "\"mirror_ambient_delta_c\":null,\"mirror_dew_margin_c\":null}}",
        ds18b20_get_temperature_c());
}

/* ── state / buttons ───────────────────────────────────────── */

static void state_buttons(char *out, size_t cap, uint32_t id, const char *session) {
    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "state", id, session, true);
    proto_buf_append(out, cap, &len,
        ",\"data\":{\"fan_mode_pressed\":%s,"
        "\"local_focus_active\":false,\"led_state\":\"ready\","
        "\"last_input_change_ms\":0}}",
        buttons_fan_mode_pressed() ? "true" : "false");
}

/* ── dispatcher ────────────────────────────────────────────── */

void proto_handle_state(const char *req, uint32_t id, char *out, size_t cap) {
    char target[PROTO_MAX_TARGET_LEN] = {0};
    char session[PROTO_MAX_SESSION_LEN] = {0};
    proto_json_get_string(req, "target", target, sizeof(target));
    get_session(req, session, sizeof(session));

    if (strcmp(target, "device") == 0) {
        state_device(out, cap, id, session);
    } else if (strcmp(target, "fan") == 0) {
        state_fan(out, cap, id, session);
    } else if (strcmp(target, "environment") == 0) {
        state_environment(out, cap, id, session);
    } else if (strcmp(target, "buttons") == 0) {
        state_buttons(out, cap, id, session);
    } else {
        size_t len = 0;
        out[0] = '\0';
        proto_env_begin(out, cap, &len, "state", id, session, false);
        proto_env_error(out, cap, &len, "INVALID_ARGUMENT", "unknown target");
    }
}
