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
#include <stddef.h>
#include "sdkconfig.h"
#include "soc/gpio_struct.h"
#include "hal/gpio_ll.h"
#include "esp_rom_gpio.h"
#include "soc/soc.h"
#include "soc/assist_debug_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/system_reg.h"
#include "bootloader_common.h"

/*
 * Fluispotter IMG-8 — watchdog program-counter capture (fault diagnostics, level 2;
 * owner go 2026-10-07).
 *
 * WHY.  The ESP-IDF light-sleep safety-net RTC watchdog resets the system without a
 * panic, a backtrace or a core dump.  The only record of what each CPU was executing
 * is the ASSIST_DEBUG trace port's PDEBUG* registers, latched by the hardware across
 * the reset.  IDF reads them in bootloader_check_wdt_reset() at ESP_LOGD (compiled
 * out here) and then RE-ARMS recording, after which the registers track the live PC
 * (measured 2026-09-25: recording live on both cores at application time).  This
 * hook runs before bootloader_init(), i.e. before that re-arm, and copies the words
 * into the bootloader's custom RTC reserve, which the application reads at boot and
 * reports on its Boot event.
 *
 * HOW, deliberately minimal: plain register READS, exactly the ones IDF's own
 * wdt_reset_info_dump() makes, in the same state (no clock enable, no reset release,
 * no write to the trace port).  The peripheral clock/reset enables are recorded too,
 * so a capture taken with the block clock-gated is recognisable.  The words land in
 * rtc_retain_mem_t.custom[], which is excluded from the block's CRC
 * (CONFIG_BOOTLOADER_CUSTOM_RESERVE_RTC_IN_CRC=n), so writing it never invalidates
 * the block; on a power-on the bootloader finds the block invalid and zeroes it,
 * custom[] included, which is the right outcome (no watchdog PC exists then).
 *
 * LAYOUT of custom[] (20 words, CONFIG_BOOTLOADER_CUSTOM_RESERVE_RTC_SIZE = 0x50):
 *   [0]  FS_PDBG_MAGIC              written last-but-not-least: the capture is valid
 *   [1]  RTC_CNTL_RESET_STATE_REG    raw reset cause, PRO [5:0], APP [11:6]
 *   [2]  SYSTEM_CPU_PERI_CLK_EN_REG  (bit 6 = ASSIST_DEBUG clock)
 *   [3]  SYSTEM_CPU_PERI_RST_EN_REG  (bit 6 = ASSIST_DEBUG held in reset)
 *   [4..10]  core 0: PDEBUGPC, INST, STATUS, DATA, LS0STAT, LS0ADDR, LS0DATA
 *   [11..17] core 1: the same seven
 *   [18..19] 0 — together with [17] these overlay where the 16-byte block of the
 *            images without the reserve used to sit (0x600FFFF0); words 18/19 are
 *            written 0 so firmware probing that old address sees an invalid block.
 */
#define FS_PDBG_MAGIC  0xFD8C0001u
#define FS_PDBG_WORDS  20

_Static_assert(CONFIG_BOOTLOADER_CUSTOM_RESERVE_RTC_SIZE == FS_PDBG_WORDS * 4,
               "custom RTC reserve must be 0x50 bytes (sdkconfig.board)");
_Static_assert(offsetof(rtc_retain_mem_t, custom) == 12,
               "rtc_retain_mem_t layout changed: bootstat.py reads custom[] at +12");

static void fluispotter_capture_pdebug(void)
{
    volatile uint32_t *c = (volatile uint32_t *)bootloader_common_get_rtc_retain_mem()->custom;
    c[0] = 0;   /* invalid while being written */
    c[1] = REG_READ(RTC_CNTL_RESET_STATE_REG);
    c[2] = REG_READ(SYSTEM_CPU_PERI_CLK_EN_REG);
    c[3] = REG_READ(SYSTEM_CPU_PERI_RST_EN_REG);
    c[4]  = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGPC_REG);
    c[5]  = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGINST_REG);
    c[6]  = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGSTATUS_REG);
    c[7]  = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGDATA_REG);
    c[8]  = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGLS0STAT_REG);
    c[9]  = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGLS0ADDR_REG);
    c[10] = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGLS0DATA_REG);
    c[11] = REG_READ(ASSIST_DEBUG_CORE_1_RCD_PDEBUGPC_REG);
    c[12] = REG_READ(ASSIST_DEBUG_CORE_1_RCD_PDEBUGINST_REG);
    c[13] = REG_READ(ASSIST_DEBUG_CORE_1_RCD_PDEBUGSTATUS_REG);
    c[14] = REG_READ(ASSIST_DEBUG_CORE_1_RCD_PDEBUGDATA_REG);
    c[15] = REG_READ(ASSIST_DEBUG_CORE_1_RCD_PDEBUGLS0STAT_REG);
    c[16] = REG_READ(ASSIST_DEBUG_CORE_1_RCD_PDEBUGLS0ADDR_REG);
    c[17] = REG_READ(ASSIST_DEBUG_CORE_1_RCD_PDEBUGLS0DATA_REG);
    c[18] = 0;
    c[19] = 0;
    c[0] = FS_PDBG_MAGIC;
}

static const uint8_t safe_low_gpios[] = { 9, 17, 8, 41 };

void bootloader_before_init(void) {
    /* IMG-8: first, before anything can disturb the trace port (see above). */
    fluispotter_capture_pdebug();
    for (unsigned i = 0; i < sizeof(safe_low_gpios); i++) {
        uint8_t g = safe_low_gpios[i];
        esp_rom_gpio_pad_select_gpio(g);      /* IO_MUX -> plain GPIO      */
        gpio_ll_pullup_dis(&GPIO, g);
        gpio_ll_pulldown_en(&GPIO, g);        /* rests LOW even un-driven  */
        gpio_ll_set_level(&GPIO, g, 0);       /* level first: no high blip */
        gpio_ll_output_enable(&GPIO, g);      /* actively driven LOW       */
    }
}
