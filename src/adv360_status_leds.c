/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT kinesis_adv360_status_leds

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/ext_power.h>
#include <dt-bindings/zmk/hid_indicators.h>
#include <zmk/battery.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators_types.h>

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/events/position_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/hid_indicators.h>
#include <zmk/keymap.h>
#define ADV360_STATUS_HAS_CENTRAL_STATE 1
#else
#define ADV360_STATUS_HAS_CENTRAL_STATE 0
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT) && IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/split/central.h>
#define ADV360_STATUS_CAN_SYNC_LAYER 1
#else
#define ADV360_STATUS_CAN_SYNC_LAYER 0
#endif

#include <zmk_keyboard_adv360/status_leds.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define STATUS_NODE DT_DRV_INST(0)
#define STATUS_STRIP_NODE DT_PHANDLE(STATUS_NODE, led_strip)
#define STATUS_LED_COUNT DT_PROP(STATUS_STRIP_NODE, chain_length)

BUILD_ASSERT(STATUS_LED_COUNT >= 3, "Adv360 status LED strip must have at least three pixels");

static const struct device *const led_strip = DEVICE_DT_GET(STATUS_STRIP_NODE);
static struct led_rgb pixels[STATUS_LED_COUNT];

static bool status_enabled = IS_ENABLED(CONFIG_ZMK_ADV360_STATUS_LEDS_ON_START);
static bool battery_preview;
static zmk_hid_indicators_t hid_indicators;
static uint8_t active_layer;

static struct k_work update_work;

#define ADV360_STP_SET_LAYER 2

static struct led_rgb scale_rgb(uint32_t hex) {
    const uint8_t brightness = CONFIG_ZMK_ADV360_STATUS_LEDS_BRIGHTNESS;

    return (struct led_rgb){
        .r = (uint8_t)((((hex >> 16) & 0xFF) * brightness) / 100),
        .g = (uint8_t)((((hex >> 8) & 0xFF) * brightness) / 100),
        .b = (uint8_t)((((hex >> 0) & 0xFF) * brightness) / 100),
    };
}

static const uint32_t layer_colors[] = {
    0x000000, 0xFFFFFF, 0x0000FF, 0x00FF00, 0xFF0000, 0xFF00FF, 0x00FFFF, 0xFFFF00,
};

static void layer_to_color_indices(uint8_t layer, uint8_t *left, uint8_t *right) {
    if (layer < ARRAY_SIZE(layer_colors)) {
        *left = layer;
        *right = layer;
        return;
    }

    *left = 1 + ((layer - ARRAY_SIZE(layer_colors)) / 6);
    *right = 1 + ((layer - ARRAY_SIZE(layer_colors)) % 6);

    if (*left <= *right) {
        *right += 1;
    }

    *left %= ARRAY_SIZE(layer_colors);
    *right %= ARRAY_SIZE(layer_colors);
}

static struct led_rgb battery_color(void) {
    const uint8_t soc = zmk_battery_state_of_charge();

    if (soc >= 80) {
        return scale_rgb(0x00FF00);
    } else if (soc >= 50) {
        return scale_rgb(0xFFFF00);
    } else if (soc >= 20) {
        return scale_rgb(0xFF8C00);
    } else {
        return scale_rgb(0xFF0000);
    }
}

static void clear_pixels(void) { memset(pixels, 0, sizeof(pixels)); }

static int set_ext_power(bool on) {
    const struct device *ext_power = device_get_binding("EXT_POWER");

    if (ext_power == NULL) {
        return 0;
    }

    return on ? ext_power_enable(ext_power) : ext_power_disable(ext_power);
}

static int flush_pixels(bool power_on) {
    struct led_rgb out[STATUS_LED_COUNT];
    int err;

    if (power_on) {
        err = set_ext_power(true);
        if (err) {
            LOG_WRN("Failed to enable Adv360 status LED power: %d", err);
        }
    }

    memcpy(out, pixels, sizeof(out));
    err = led_strip_update_rgb(led_strip, out, ARRAY_SIZE(out));
    if (err) {
        LOG_ERR("Failed to update Adv360 status LEDs: %d", err);
        return err;
    }

    if (!power_on) {
        err = set_ext_power(false);
        if (err) {
            LOG_WRN("Failed to disable Adv360 status LED power: %d", err);
        }
    }

    return 0;
}

static void render_status(void) {
    uint8_t left_color;
    uint8_t right_color;

    clear_pixels();

    if (!status_enabled && !battery_preview) {
        return;
    }

    if (battery_preview) {
        struct led_rgb color = battery_color();
        for (int i = 0; i < ARRAY_SIZE(pixels); i++) {
            pixels[i] = color;
        }
        return;
    }

    layer_to_color_indices(active_layer, &left_color, &right_color);

#if IS_ENABLED(CONFIG_BOARD_ADV360_LEFT)
    pixels[0] = (hid_indicators & HID_INDICATOR_CAPS_LOCK) ? scale_rgb(0xFFFFFF)
                                                           : scale_rgb(0x000000);
    pixels[1] = scale_rgb(0x000000);
    pixels[2] = scale_rgb(layer_colors[left_color]);
#elif IS_ENABLED(CONFIG_BOARD_ADV360_RIGHT)
    pixels[0] = scale_rgb(layer_colors[right_color]);
    pixels[1] = (hid_indicators & HID_INDICATOR_SCROLL_LOCK) ? scale_rgb(0xFFFFFF)
                                                             : scale_rgb(0x000000);
    pixels[2] = (hid_indicators & HID_INDICATOR_NUM_LOCK) ? scale_rgb(0xFFFFFF)
                                                          : scale_rgb(0x000000);
#else
    pixels[0] = scale_rgb(layer_colors[left_color]);
#endif
}

static void update_work_handler(struct k_work *work) {
    render_status();
    flush_pixels(status_enabled || battery_preview);
}

static void schedule_update(void) { k_work_submit(&update_work); }

#if ADV360_STATUS_CAN_SYNC_LAYER
static void sync_layer_to_peripherals(void) {
    struct zmk_behavior_binding binding = {
        .behavior_dev = "stp",
        .param1 = ADV360_STP_SET_LAYER,
        .param2 = active_layer,
    };
    struct zmk_behavior_binding_event event = {
        .position = 0,
        .timestamp = k_uptime_get(),
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
    };

    for (int i = 0; i < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; i++) {
        zmk_split_central_invoke_behavior(i, &binding, event, true);
    }
}
#else
static void sync_layer_to_peripherals(void) {}
#endif

int adv360_status_leds_show_battery(bool show) {
    battery_preview = show;
    schedule_update();
    return 0;
}

int adv360_status_leds_toggle(void) {
    status_enabled = !status_enabled;
    battery_preview = false;
    schedule_update();
    return 0;
}

int adv360_status_leds_set_layer(uint8_t layer) {
    active_layer = layer;
    schedule_update();
    return 0;
}

static int adv360_status_leds_listener(const zmk_event_t *eh) {
    const struct zmk_hid_indicators_changed *hid_ev = as_zmk_hid_indicators_changed(eh);

    if (hid_ev != NULL) {
        hid_indicators = hid_ev->indicators;
        schedule_update();
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (as_zmk_battery_state_changed(eh) != NULL && battery_preview) {
        schedule_update();
        return ZMK_EV_EVENT_BUBBLE;
    }

#if ADV360_STATUS_HAS_CENTRAL_STATE
    if (as_zmk_layer_state_changed(eh) != NULL) {
        active_layer = zmk_keymap_highest_layer_active();
        schedule_update();
        sync_layer_to_peripherals();
        return ZMK_EV_EVENT_BUBBLE;
    }
#endif

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(adv360_status_leds, adv360_status_leds_listener);
ZMK_SUBSCRIPTION(adv360_status_leds, zmk_hid_indicators_changed);
ZMK_SUBSCRIPTION(adv360_status_leds, zmk_battery_state_changed);
#if ADV360_STATUS_HAS_CENTRAL_STATE
ZMK_SUBSCRIPTION(adv360_status_leds, zmk_layer_state_changed);
#endif

static int adv360_status_leds_init(void) {
    if (!device_is_ready(led_strip)) {
        LOG_ERR("Adv360 status LED strip device is not ready");
        return -ENODEV;
    }

    k_work_init(&update_work, update_work_handler);

#if ADV360_STATUS_HAS_CENTRAL_STATE
    hid_indicators = zmk_hid_indicators_get_current_profile();
    active_layer = zmk_keymap_highest_layer_active();
#endif

    schedule_update();
    sync_layer_to_peripherals();
    return 0;
}

SYS_INIT(adv360_status_leds_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
