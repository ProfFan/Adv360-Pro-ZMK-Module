# SPDX-License-Identifier: MIT

zephyr_include_directories(include)

target_sources_ifdef(CONFIG_ZMK_ADV360_STATUS_LEDS app PRIVATE
  src/adv360_status_leds.c
  src/behaviors/behavior_adv360_status_tap.c
)
