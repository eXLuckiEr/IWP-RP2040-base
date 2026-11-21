/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef SCPI_I2C_H
#define SCPI_I2C_H

/* Модуль: диагностика шины I2C через SCPI.
 *   I2C:SCAN?                 -> список найденных 7-битных адресов
 *   I2C:WRITE a,reg,val       записать байт в регистр
 *   I2C:READ? a,reg           -> прочитать байт из регистра
 * Пины и скорость: I2C_TOOLS_* в config/board_config.h */

#include <stdbool.h>
#include "scpi/scpi.h"

bool i2c_tools_init(void);

scpi_result_t scpi_i2c_scan_q(scpi_t *context);
scpi_result_t scpi_i2c_write(scpi_t *context);
scpi_result_t scpi_i2c_read_q(scpi_t *context);

#define I2C_TOOLS_SCPI_COMMANDS \
    { .pattern = "I2C:SCAN?",  .callback = scpi_i2c_scan_q, }, \
    { .pattern = "I2C:WRITE",  .callback = scpi_i2c_write,  }, \
    { .pattern = "I2C:READ?",  .callback = scpi_i2c_read_q, },

#endif
