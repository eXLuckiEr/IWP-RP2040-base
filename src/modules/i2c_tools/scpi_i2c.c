/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "scpi_i2c.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "config/board_config.h"

#define I2C_TIMEOUT_US  10000

bool i2c_tools_init(void)
{
    i2c_init(I2C_TOOLS_PORT, I2C_TOOLS_BAUD);
    gpio_set_function(I2C_TOOLS_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_TOOLS_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_TOOLS_SDA);
    gpio_pull_up(I2C_TOOLS_SCL);
    return true;
}

static bool reserved_addr(uint8_t addr)
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

/* Читает параметр и проверяет диапазон; при ошибке кладёт её в очередь SCPI. */
static bool param_u8(scpi_t *context, uint32_t max, uint8_t *out)
{
    uint32_t v;
    if (!SCPI_ParamUInt32(context, &v, TRUE)) {
        return false;
    }
    if (v > max) {
        SCPI_ErrorPush(context, SCPI_ERROR_DATA_OUT_OF_RANGE);
        return false;
    }
    *out = (uint8_t)v;
    return true;
}

/* Ответ на сканирование — несколько миллисекунд, это допустимо для
 * SCPI-колбэка. Если шина станет медленнее — вынесите в job. */
scpi_result_t scpi_i2c_scan_q(scpi_t *context)
{
    for (uint8_t addr = 0; addr < 0x80; addr++) {
        if (reserved_addr(addr)) {
            continue;
        }
        uint8_t dummy = 0;
        if (i2c_write_timeout_us(I2C_TOOLS_PORT, addr, &dummy, 1, false, I2C_TIMEOUT_US) >= 0) {
            SCPI_ResultUInt32(context, addr);
        }
    }
    return SCPI_RES_OK;
}

scpi_result_t scpi_i2c_write(scpi_t *context)
{
    uint8_t addr, reg, val;
    if (!param_u8(context, 0x7F, &addr) ||
        !param_u8(context, 0xFF, &reg)  ||
        !param_u8(context, 0xFF, &val)) {
        return SCPI_RES_ERR;
    }
    uint8_t buf[2] = { reg, val };
    if (i2c_write_timeout_us(I2C_TOOLS_PORT, addr, buf, 2, false, I2C_TIMEOUT_US) != 2) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }
    return SCPI_RES_OK;
}

scpi_result_t scpi_i2c_read_q(scpi_t *context)
{
    uint8_t addr, reg, val = 0;
    if (!param_u8(context, 0x7F, &addr) ||
        !param_u8(context, 0xFF, &reg)) {
        return SCPI_RES_ERR;
    }
    if (i2c_write_timeout_us(I2C_TOOLS_PORT, addr, &reg, 1, true, I2C_TIMEOUT_US) != 1 ||
        i2c_read_timeout_us(I2C_TOOLS_PORT, addr, &val, 1, false, I2C_TIMEOUT_US) != 1) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }
    SCPI_ResultUInt32(context, val);
    return SCPI_RES_OK;
}
