/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef GPIO_INFO_H
#define GPIO_INFO_H

/* ===========================================================================
 *  gpio_info — только чтение: во что сконфигурирован каждый пин GPIO и что на нём.
 *
 *    GPIO:STATe? <pin>   -> "<функция>",<IN|OUT>,<0|1>,<NONE|UP|DOWN|BOTH>
 *                           пример: "SPI0_SCK",OUT,1,NONE
 *    GPIO:LEVel? <pin>   -> 0|1   уровень на ножке
 *    GPIO:MAP?           -> по одной строке на каждый пин 0..29:
 *                           "<pin>,<функция>,<IN|OUT>,<0|1>,<подтяжка>"
 *
 *  Функция — это то, куда пин ПОДКЛЮЧЁН мультиплексором (SPI0_SCK, UART1_TX,
 *  I2C1_SDA, PWM3_A, PIO0, SIO, ADC0...). Включена ли сама периферия, из этого
 *  не следует. Направление и уровень берутся из регистра status пина, поэтому
 *  верны и для пинов, которыми управляет периферия, а не код.
 * ======================================================================== */

#include <stddef.h>
#include <stdbool.h>
#include "scpi/scpi.h"

/* Чистая функция разбора (без обращения к железу): имя сигнала по номеру пина,
 * номеру функции FUNCSEL (0..9, 31) и состоянию цифрового входа. */
void iwp_gpio_signal_name(unsigned pin, unsigned funcsel, bool input_enabled,
                          char *out, size_t out_size);

scpi_result_t scpi_gpio_state_q(scpi_t *context);
scpi_result_t scpi_gpio_level_q(scpi_t *context);
scpi_result_t scpi_gpio_map_q(scpi_t *context);

#define GPIO_INFO_SCPI_COMMANDS \
    { .pattern = "GPIO:STATe?", .callback = scpi_gpio_state_q, }, \
    { .pattern = "GPIO:LEVel?", .callback = scpi_gpio_level_q, }, \
    { .pattern = "GPIO:MAP?",   .callback = scpi_gpio_map_q,   },

#endif /* GPIO_INFO_H */
