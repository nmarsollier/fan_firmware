/* Proto — proto_control.c — CONTROL: fan mode/speed and minimal device
 * session/heartbeat.
 */
#include "proto_internal.h"

#include <string.h>

#include "esp_timer.h"

static void get_session(const char *req, char *out, size_t cap) {
    proto_json_get_string(req, "session", out, cap);
}

static void emit_err(char *out, size_t cap, const char *type, uint32_t id,
                     const char *session, const char *code, const char *message) {
    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, type, id, session, false);
    proto_env_error(out, cap, &len, code, message);
}

/* ── control / fan / mode ──────────────────────────────────── */

static void handle_fan_mode(const char *req, uint32_t id, const char *session,
                            char *out, size_t cap) {
    char params[256];
    char mode[16] = {0};
    FanMode m;

    if (proto_json_get_object(req, "params", params, sizeof(params)) &&
        proto_json_get_string(params, "value", mode, sizeof(mode)) &&
        fan_mode_from_string(mode, &m)) {
        fan_set_mode(m);

        size_t len = 0;
        out[0] = '\0';
        proto_env_begin(out, cap, &len, "control", id, session, true);
        proto_buf_append(out, cap, &len,
            ",\"data\":{\"mode\":\"%s\"}}",
            fan_mode_to_string(fan_get_mode()));
        return;
    }
    emit_err(out, cap, "control", id, session, "INVALID_ARGUMENT",
             "fan mode requires params.value (off|auto|manual)");
}

/* ── control / fan / speed ─────────────────────────────────── */

static void handle_fan_speed(const char *req, uint32_t id, const char *session,
                             char *out, size_t cap) {
    char params[256];
    int value = -1;

    if (proto_json_get_object(req, "params", params, sizeof(params)) &&
        proto_json_get_int(params, "value", &value)) {
        if (value < 1 || value > 100) {
            emit_err(out, cap, "control", id, session, "OUT_OF_RANGE",
                     "fan speed must be 1..100");
            return;
        }
        fan_set_speed((uint8_t) value);

        size_t len = 0;
        out[0] = '\0';
        proto_env_begin(out, cap, &len, "control", id, session, true);
        proto_buf_append(out, cap, &len,
            ",\"data\":{\"speed\":%d}}",
            value);
        return;
    }
    emit_err(out, cap, "control", id, session, "INVALID_ARGUMENT",
             "fan speed requires params.value (1..100)");
}

/* ── control / device / session_open & heartbeat (minimal) ─── */

static void handle_session_open(uint32_t id, const char *session, char *out, size_t cap) {
    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "control", id, session, true);
    proto_buf_append(out, cap, &len,
        ",\"data\":{\"session\":\"%s\",\"boot_id\":\"", session);
    char boot[9];
    proto_boot_id_str(boot, sizeof(boot));
    proto_buf_append(out, cap, &len, "%s", boot);
    proto_buf_append(out, cap, &len, "\",\"lease_ms\":3000,\"last_event_seq\":0}}");
}

static void handle_heartbeat(uint32_t id, const char *session, char *out, size_t cap) {
    size_t len = 0;
    out[0] = '\0';
    proto_env_begin(out, cap, &len, "control", id, session, true);
    proto_buf_append(out, cap, &len,
        ",\"data\":{\"uptime_ms\":%llu,\"lease_remaining_ms\":3000}}",
        (unsigned long long) (esp_timer_get_time() / 1000));
}

/* ── dispatcher ────────────────────────────────────────────── */

void proto_handle_control(const char *req, uint32_t id, char *out, size_t cap) {
    char target[PROTO_MAX_TARGET_LEN] = {0};
    char op[PROTO_MAX_ACTION_LEN] = {0};
    char session[PROTO_MAX_SESSION_LEN] = {0};

    proto_json_get_string(req, "target", target, sizeof(target));
    proto_json_get_string(req, "op", op, sizeof(op));
    get_session(req, session, sizeof(session));

    if (strcmp(target, "fan") == 0 && strcmp(op, "mode") == 0) {
        handle_fan_mode(req, id, session, out, cap);
    } else if (strcmp(target, "fan") == 0 && strcmp(op, "speed") == 0) {
        handle_fan_speed(req, id, session, out, cap);
    } else if (strcmp(target, "device") == 0 && strcmp(op, "session_open") == 0) {
        handle_session_open(id, session, out, cap);
    } else if (strcmp(target, "device") == 0 && strcmp(op, "heartbeat") == 0) {
        handle_heartbeat(id, session, out, cap);
    } else {
        emit_err(out, cap, "control", id, session, "NOT_SUPPORTED",
                 "unknown control operation");
    }
}
