/*
 * Fluispotter actuator power-on safety (SWR-276) — second-stage bootloader hook.
 *
 * Field fault 2026-07-16: the AC pump ran at FULL POWER from the moment power
 * was applied (shipping-magnet removal) until firmware claimed the pins.  The
 * DAC7574 powers up at zero-scale, which is full drive in the board's
 * inverted signal convention (power = 4095 - register), and the AC driver's
 * enable input is biased ON while the ESP32-S3 pin floats (datasheet v1.6
 * Table 2-1: GPIO9/17 have NO internal pulls at reset).  boot.py closes the
 * window at ~0.5 s (VM init); this hook closes it at the second-stage
 * bootloader, ~tens of ms after power-on — the earliest programmable moment.
 *
 * Drive every ACTUATOR enable low and latch an internal pull-down:
 *   GPIO 9   AC pump enable
 *   GPIO 17  blood pump enable
 *   GPIO 8   valve enable
 *   GPIO 41  tape reel enable
 * (Encoder enables 14/18 are sensor power, not actuators — left to boot.py.)
 *
 * Bootloader context: only ROM + hal/gpio_ll register ops, no drivers, no
 * logging (keeps boot time unchanged).  The remaining exposure is the mask-
 * ROM window (~10-50 ms), closable only by a board-level pull-down (queued
 * for the next hardware revision).
 */
#include <stdint.h>
#include "sdkconfig.h"
#include "soc/gpio_struct.h"
#include "hal/gpio_ll.h"
#include "esp_rom_gpio.h"

static const uint8_t safe_low_gpios[] = { 9, 17, 8, 41 };

void bootloader_before_init(void) {
    for (unsigned i = 0; i < sizeof(safe_low_gpios); i++) {
        uint8_t g = safe_low_gpios[i];
        esp_rom_gpio_pad_select_gpio(g);      /* IO_MUX -> plain GPIO      */
        gpio_ll_pullup_dis(&GPIO, g);
        gpio_ll_pulldown_en(&GPIO, g);        /* rests LOW even un-driven  */
        gpio_ll_set_level(&GPIO, g, 0);       /* level first: no high blip */
        gpio_ll_output_enable(&GPIO, g);      /* actively driven LOW       */
    }
}
