/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "sysinfo.h"

#include <FreeRTOS.h>
#include <task.h>

#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"

#include "config/board_config.h"

#define ADC_TEMP_CHANNEL    4
#define ADC_FULL_SCALE      4096.0f     /* 12 бит */

static char s_uid[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];

bool iwp_sysinfo_init(void)
{
    pico_get_unique_board_id_string(s_uid, sizeof(s_uid));

    adc_init();
    adc_set_temp_sensor_enabled(true);
    return true;
}

const char *iwp_sysinfo_uid(void)
{
    return s_uid;
}

float iwp_sysinfo_read_temperature_c(void)
{
    adc_select_input(ADC_TEMP_CHANNEL);

    uint32_t sum = 0;
    for (int i = 0; i < SYSINFO_TEMP_AVERAGE; i++) {
        sum += adc_read();
    }
    float raw  = (float)sum / (float)SYSINFO_TEMP_AVERAGE;
    float volt = raw * SYSINFO_ADC_VREF_VOLTS / ADC_FULL_SCALE;

    /* Формула из даташита RP2040 (раздел 4.9.5). */
    return 27.0f - (volt - 0.706f) / 0.001721f + SYSINFO_TEMP_OFFSET_C;
}

scpi_result_t scpi_sys_temperature_q(scpi_t *context)
{
    SCPI_ResultFloat(context, iwp_sysinfo_read_temperature_c());
    return SCPI_RES_OK;
}

scpi_result_t scpi_sys_uid_q(scpi_t *context)
{
    SCPI_ResultText(context, s_uid);
    return SCPI_RES_OK;
}

scpi_result_t scpi_sys_clock_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, clock_get_hz(clk_sys));
    return SCPI_RES_OK;
}

scpi_result_t scpi_sys_uptime_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, (uint32_t)(time_us_64() / 1000000u));
    return SCPI_RES_OK;
}

scpi_result_t scpi_sys_heap_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, (uint32_t)xPortGetFreeHeapSize());
    SCPI_ResultUInt32(context, (uint32_t)xPortGetMinimumEverFreeHeapSize());
    return SCPI_RES_OK;
}
