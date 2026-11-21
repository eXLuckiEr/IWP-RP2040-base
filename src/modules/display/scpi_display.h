#ifndef SCPI_DISPLAY_H
#define SCPI_DISPLAY_H

/* Модуль: OLED SSD1309 (драйвер libdriver в modules/display/driver).
 *   DISPlay:STATe 0|1
 *   DISPlay:CLEar
 *   DISPlay:WRITe "текст"
 * ВНИМАНИЕ: пины SPI дисплея заданы в driver/driver_ssd1309_interface.c;
 * они совпадают с пинами примера ADC (SPI0, GP16-19) — разведите их. */

#include <stdbool.h>
#include "scpi/scpi.h"

bool display_init(void);

scpi_result_t scpi_display_state(scpi_t *context);
scpi_result_t scpi_display_clear(scpi_t *context);
scpi_result_t scpi_display_write(scpi_t *context);

#define DISPLAY_SCPI_COMMANDS \
    { .pattern = "DISPlay:STATe", .callback = scpi_display_state, }, \
    { .pattern = "DISPlay:CLEar", .callback = scpi_display_clear, }, \
    { .pattern = "DISPlay:WRITe", .callback = scpi_display_write, },

#endif
