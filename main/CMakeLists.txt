# main component — sources and REQUIRES grouped by module.
idf_component_register(
        SRCS
        "main.c"

        "board/board.c"

        "led/led_init.c"
        "led/led_state.c"
        "led/led_beacon.c"
        "led/led_update.c"

        "ds18b20/ds18b20_init.c"
        "ds18b20/ds18b20_read.c"

        "fan/fan.c"
        "buttons/buttons.c"

        "usb_cdc/usb_cdc_init.c"
        "usb_cdc/usb_cdc_rx.c"
        "usb_cdc/usb_cdc_tx.c"

        "proto/proto_init.c"
        "proto/proto_frame.c"
        "proto/proto_json.c"
        "proto/proto_dispatch.c"
        "proto/proto_session.c"
        "proto/proto_capabilities.c"
        "proto/proto_state.c"
        "proto/proto_action.c"
        "proto/proto_config.c"
        "proto/proto_control.c"

        "runtime/runtime_setup.c"
        "runtime/runtime_loop.c"

        INCLUDE_DIRS
        "."
        "config"
        "board"
        "led"
        "ds18b20"
        "fan"
        "buttons"
        "usb_cdc"
        "proto"
        "runtime"

        REQUIRES
        esp_timer
        nvs_flash
        esp_driver_gpio
        esp_driver_ledc
        esp_adc
        esp_driver_rmt
)
