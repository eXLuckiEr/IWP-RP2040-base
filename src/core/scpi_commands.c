/*-
 * BSD 2-Clause License
 *
 * Copyright (c) 2012-2018, Jan Breuer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Файл основан на примере libscpi (examples/test-parser/scpi-def.*), BSD 2-Clause,
 * изменён: адаптация под FreeRTOS/IWP. Уведомление об авторских правах выше
 * обязательно сохранять. */

/*
 * Таблица SCPI-команд.
 *
 * Базовые команды (IEEE 488.2 + обязательный минимум SCPI-1999) всегда
 * включены. Команды примеров подключаются макросами из соответствующих
 * заголовков и только если пример включён в CMake (-DENABLE_...=ON).
 *
 * Свои команды добавляйте в конец таблицы (перед SCPI_CMD_LIST_END).
 */

#include "scpi_port.h"

#include <FreeRTOS.h>
#include "config/board_config.h"
#include "core/job.h"
#include "core/sysinfo.h"
#include "core/gpio_info.h"

#if APP_ADC_DUMP
#include "app/adc_dump.h"
#endif
#if APP_I2C_TOOLS
#include "modules/i2c_tools/scpi_i2c.h"
#endif
#if APP_DISPLAY
#include "modules/display/scpi_display.h"
#endif

/* ABORt — остановить все фоновые задачи. Возвращается, когда все остановлены. */
static scpi_result_t scpi_abort(scpi_t *context)
{
    if (!iwp_job_stop_all(pdMS_TO_TICKS(500))) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }
    return SCPI_RES_OK;
}

const scpi_command_t scpi_commands[] = {
    /* IEEE 488.2 */
    { .pattern = "*CLS",  .callback = SCPI_CoreCls,  },
    { .pattern = "*ESE",  .callback = SCPI_CoreEse,  },
    { .pattern = "*ESE?", .callback = SCPI_CoreEseQ, },
    { .pattern = "*ESR?", .callback = SCPI_CoreEsrQ, },
    { .pattern = "*IDN?", .callback = SCPI_CoreIdnQ, },
    { .pattern = "*OPC",  .callback = SCPI_CoreOpc,  },
    { .pattern = "*OPC?", .callback = SCPI_CoreOpcQ, },
    { .pattern = "*RST",  .callback = SCPI_CoreRst,  },
    { .pattern = "*SRE",  .callback = SCPI_CoreSre,  },
    { .pattern = "*SRE?", .callback = SCPI_CoreSreQ, },
    { .pattern = "*STB?", .callback = SCPI_CoreStbQ, },
    { .pattern = "*TST?", .callback = SCPI_CoreTstQ, },
    { .pattern = "*WAI",  .callback = SCPI_CoreWai,  },

    /* Обязательный минимум SCPI */
    { .pattern = "SYSTem:ERRor[:NEXT]?", .callback = SCPI_SystemErrorNextQ, },
    { .pattern = "SYSTem:ERRor:COUNt?",  .callback = SCPI_SystemErrorCountQ, },
    { .pattern = "SYSTem:VERSion?",      .callback = SCPI_SystemVersionQ, },
    { .pattern = "STATus:PRESet",        .callback = SCPI_StatusPreset, },

    /* Информация о самом чипе (core/sysinfo.c) */
    SYSINFO_SCPI_COMMANDS

    /* Состояние пинов GPIO (core/gpio_info.c) */
    GPIO_INFO_SCPI_COMMANDS

    /* Остановка всего, что работает в фоне */
    { .pattern = "ABORt", .callback = scpi_abort, },

    /* ---- Примеры / модули ---- */
#if APP_ADC_DUMP
    ADC_DUMP_SCPI_COMMANDS
#endif
#if APP_I2C_TOOLS
    I2C_TOOLS_SCPI_COMMANDS
#endif
#if APP_DISPLAY
    DISPLAY_SCPI_COMMANDS
#endif

    SCPI_CMD_LIST_END
};
