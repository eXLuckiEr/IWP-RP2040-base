/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef ADC_DUMP_H
#define ADC_DUMP_H

/* ===========================================================================
 *  Пример: потоковый дамп SPI-АЦП на хост (SPI -> DMA ping-pong -> USB).
 *
 *  SCPI:
 *    ADC:DUMP:STARt [count]      запустить; count = число отсчётов, без
 *                                параметра (или 0) — до команды STOP
 *    ADC:DUMP:STOP               остановить; возвращается, когда поток закончился
 *    ADC:DUMP:STATe?             IDLE | RUN
 *    ADC:DUMP:FORMat ASCii|BINary   (только в IDLE)
 *    ADC:DUMP:RATE <Гц>             (только в IDLE)
 *    ADC:DUMP:STATistics?        отсчётов,чанков,потеряно_чанков,переполнений_DMA
 *    ABORt  /  *RST  /  байт 0x03 в потоке   тоже останавливают дамп
 *
 *  Формат ASCII:  ">Val:<число>\r\n" на каждый отсчёт.
 *  Формат BINary: пакеты  A5 5A | seq (u16 LE) | n (u16 LE) | n * u16 LE
 *  seq растёт на 1 с каждым пакетом — по разрыву хост видит потерю данных.
 *
 *  Как отделить конец дампа от других ответов:
 *      ADC:DUMP:STOP;*OPC?
 *  STOP возвращается после остановки потока, поэтому всё до ответа "1"
 *  относится к дампу.
 * ======================================================================== */

#include <stdbool.h>
#include "scpi/scpi.h"

bool adc_dump_init(void);

scpi_result_t adc_dump_scpi_start(scpi_t *context);
scpi_result_t adc_dump_scpi_stop(scpi_t *context);
scpi_result_t adc_dump_scpi_state_q(scpi_t *context);
scpi_result_t adc_dump_scpi_format(scpi_t *context);
scpi_result_t adc_dump_scpi_format_q(scpi_t *context);
scpi_result_t adc_dump_scpi_rate(scpi_t *context);
scpi_result_t adc_dump_scpi_rate_q(scpi_t *context);
scpi_result_t adc_dump_scpi_stats_q(scpi_t *context);

#define ADC_DUMP_SCPI_COMMANDS \
    { .pattern = "ADC:DUMP:STARt",       .callback = adc_dump_scpi_start,    }, \
    { .pattern = "ADC:DUMP:STOP",        .callback = adc_dump_scpi_stop,     }, \
    { .pattern = "ADC:DUMP:STATe?",      .callback = adc_dump_scpi_state_q,  }, \
    { .pattern = "ADC:DUMP:FORMat",      .callback = adc_dump_scpi_format,   }, \
    { .pattern = "ADC:DUMP:FORMat?",     .callback = adc_dump_scpi_format_q, }, \
    { .pattern = "ADC:DUMP:RATE",        .callback = adc_dump_scpi_rate,     }, \
    { .pattern = "ADC:DUMP:RATE?",       .callback = adc_dump_scpi_rate_q,   }, \
    { .pattern = "ADC:DUMP:STATistics?", .callback = adc_dump_scpi_stats_q,  },

#endif /* ADC_DUMP_H */
