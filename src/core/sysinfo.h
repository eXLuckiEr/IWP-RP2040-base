#ifndef SYSINFO_H
#define SYSINFO_H

/* ===========================================================================
 *  sysinfo — данные о самом чипе RP2040.
 *
 *    MEASure:TEMPerature?   температура кристалла, °C (встроенный датчик,
 *                           вход 4 АЦП). Точность порядка ±2…5 °C — годится
 *                           для контроля перегрева и тренда, не как термометр.
 *    SYSTem:UID?            уникальный 64-битный ID flash (16 hex-символов)
 *    SYSTem:CLOCk?          частота clk_sys, Гц
 *    SYSTem:UPTime?         секунд с момента включения
 *    SYSTem:HEAP?           свободно байт кучи FreeRTOS, минимум за всё время
 *
 *  ВАЖНО: АЦП RP2040 один, мультиплексор общий. Если ваш код тоже читает
 *  adc_select_input()/adc_read() из другой задачи, оберните оба места одним
 *  мьютексом — иначе показания перепутаются.
 * ======================================================================== */

#include <stdbool.h>
#include "scpi/scpi.h"

/* Вызывать из main() ДО vTaskStartScheduler(): чтение UID идёт через flash,
 * а на втором ядре в это время ещё ничего не исполняется. */
bool sysinfo_init(void);

/* Температура кристалла, °C. */
float sysinfo_read_temperature_c(void);

scpi_result_t scpi_sys_temperature_q(scpi_t *context);
scpi_result_t scpi_sys_uid_q(scpi_t *context);
scpi_result_t scpi_sys_clock_q(scpi_t *context);
scpi_result_t scpi_sys_uptime_q(scpi_t *context);
scpi_result_t scpi_sys_heap_q(scpi_t *context);

#define SYSINFO_SCPI_COMMANDS \
    { .pattern = "MEASure:TEMPerature?", .callback = scpi_sys_temperature_q, }, \
    { .pattern = "SYSTem:UID?",          .callback = scpi_sys_uid_q,         }, \
    { .pattern = "SYSTem:CLOCk?",        .callback = scpi_sys_clock_q,       }, \
    { .pattern = "SYSTem:UPTime?",       .callback = scpi_sys_uptime_q,      }, \
    { .pattern = "SYSTem:HEAP?",         .callback = scpi_sys_heap_q,        },

#endif /* SYSINFO_H */
