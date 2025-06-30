/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 "Andreas Valder" <andreas.valder@serioese.gmbh>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/runtime.h"

#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3

#include "ulp_common.h"
#include "ulp_riscv.h"

typedef struct _esp32_ulp_riscv_obj_t {
    mp_obj_base_t base;
} esp32_ulp_riscv_obj_t;

const mp_obj_type_t esp32_ulp_riscv_type;

// singleton ULP object
static const esp32_ulp_riscv_obj_t esp32_ulp_riscv_obj = {{&esp32_ulp_riscv_type}};

static mp_obj_t esp32_ulp_riscv_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // return constant object
    return (mp_obj_t)&esp32_ulp_riscv_obj;
}

static mp_obj_t esp32_ulp_riscv_set_wakeup_period(mp_obj_t self_in, mp_obj_t period_us_in) {
    mp_uint_t period_us = mp_obj_get_int(period_us_in);
    int _errno = ulp_set_wakeup_period(0, period_us);
    if (_errno != ESP_OK) {
        mp_raise_OSError(_errno);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(esp32_ulp_riscv_set_wakeup_period_obj, esp32_ulp_riscv_set_wakeup_period);

static mp_obj_t esp32_ulp_riscv_load_binary(mp_obj_t self_in, mp_obj_t program_binary_in) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(program_binary_in, &bufinfo, MP_BUFFER_READ);

    int _errno = ulp_riscv_load_binary(bufinfo.buf, bufinfo.len);
    if (_errno != ESP_OK) {
        mp_raise_OSError(_errno);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(esp32_ulp_riscv_load_binary_obj, esp32_ulp_riscv_load_binary);

static mp_obj_t esp32_ulp_riscv_run(mp_obj_t self_in) {
    int _errno = ulp_riscv_run();
    if (_errno != ESP_OK) {
        mp_raise_OSError(_errno);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(esp32_ulp_riscv_run_obj, esp32_ulp_riscv_run);

static const mp_rom_map_elem_t esp32_ulp_riscv_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_wakeup_period), MP_ROM_PTR(&esp32_ulp_riscv_set_wakeup_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_load_binary), MP_ROM_PTR(&esp32_ulp_riscv_load_binary_obj) },
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&esp32_ulp_riscv_run_obj) },
    { MP_ROM_QSTR(MP_QSTR_RESERVE_MEM), MP_ROM_INT(CONFIG_ULP_COPROC_RESERVE_MEM) },
};
static MP_DEFINE_CONST_DICT(esp32_ulp_riscv_locals_dict, esp32_ulp_riscv_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    esp32_ulp_riscv_type,
    MP_QSTR_RVULP,
    MP_TYPE_FLAG_NONE,
    make_new, esp32_ulp_riscv_make_new,
    locals_dict, &esp32_ulp_riscv_locals_dict
    );

#endif // CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
