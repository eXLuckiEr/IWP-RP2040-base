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
    if (!job_stop_all(pdMS_TO_TICKS(500))) {
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
