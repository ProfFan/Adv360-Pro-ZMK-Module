#pragma once

#include <stdbool.h>
#include <stdint.h>

int adv360_status_leds_show_battery(bool show);
int adv360_status_leds_toggle(void);
int adv360_status_leds_set_layer(uint8_t layer);
