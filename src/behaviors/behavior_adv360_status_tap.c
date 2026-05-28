/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT kinesis_behavior_adv360_status_tap

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <dt-bindings/zmk/stp.h>

#include <zmk_keyboard_adv360/status_leds.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ADV360_STP_SET_LAYER 2

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static const struct behavior_parameter_value_metadata param_values[] = {
    {
        .value = STP_BAT,
        .display_name = "Battery Level",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
    },
    {
        .value = STP_TOG,
        .display_name = "Toggle Status LEDs",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_VALUE,
    },
};

static const struct behavior_parameter_metadata_set param_metadata_set[] = {{
    .param1_values = param_values,
    .param1_values_len = ARRAY_SIZE(param_values),
}};

static const struct behavior_parameter_metadata metadata = {
    .sets_len = ARRAY_SIZE(param_metadata_set),
    .sets = param_metadata_set,
};

#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case STP_BAT:
        return adv360_status_leds_show_battery(true);
    case STP_TOG:
        return adv360_status_leds_toggle();
    case ADV360_STP_SET_LAYER:
        return adv360_status_leds_set_layer(binding->param2);
    default:
        LOG_ERR("Unknown Adv360 status command: %d", binding->param1);
        return -ENOTSUP;
    }
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case STP_BAT:
        return adv360_status_leds_show_battery(false);
    case STP_TOG:
        return ZMK_BEHAVIOR_OPAQUE;
    default:
        return -ENOTSUP;
    }
}

static const struct behavior_driver_api behavior_adv360_status_tap_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &metadata,
#endif
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_adv360_status_tap_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
