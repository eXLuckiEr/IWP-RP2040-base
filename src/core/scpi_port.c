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

#include "scpi_port.h"

#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include "config/board_config.h"
#include "core/job.h"
#include "core/sysinfo.h"
#include "core/transport.h"

static char          s_input_buffer[SCPI_INPUT_BUFFER_LENGTH];
static scpi_error_t  s_error_queue[SCPI_ERROR_QUEUE_SIZE];
static scpi_t        s_context;

/* ---- Буферизация ответа ------------------------------------------------- *
 * libscpi пишет ответ кусками (значение, разделитель, перевод строки).
 * Если слать каждый кусок отдельно, между ними может вклиниться чанк дампа.
 * Поэтому копим ответ и отправляем одним iwp_transport_write() на SCPI_Flush().
 * ------------------------------------------------------------------------ */
static char   s_resp[SCPI_RESPONSE_BUFFER_BYTES];
static size_t s_resp_len;

static void response_flush(void)
{
    if (s_resp_len > 0) {
        iwp_transport_write(s_resp, s_resp_len, pdMS_TO_TICKS(SCPI_RESPONSE_TIMEOUT_MS));
        s_resp_len = 0;
    }
}

static size_t scpi_cb_write(scpi_t *context, const char *data, size_t len)
{
    (void)context;
    size_t total = len;

    while (len > 0) {
        size_t room = sizeof(s_resp) - s_resp_len;
        if (room == 0) {
            response_flush();       /* очень длинный ответ — уходит частями */
            room = sizeof(s_resp);
        }
        size_t n = (len < room) ? len : room;
        memcpy(s_resp + s_resp_len, data, n);
        s_resp_len += n;
        data += n;
        len -= n;
    }
    return total;
}

static scpi_result_t scpi_cb_flush(scpi_t *context)
{
    (void)context;
    response_flush();
    return SCPI_RES_OK;
}

static int scpi_cb_error(scpi_t *context, int_fast16_t err)
{
    (void)context;
#if SCPI_ECHO_ERRORS
    char msg[96];
    int n = snprintf(msg, sizeof(msg), "**ERROR: %d, \"%s\"\r\n",
                     (int)err, SCPI_ErrorTranslate(err));
    if (n > 0) {
        iwp_transport_write(msg, (size_t)n, pdMS_TO_TICKS(SCPI_RESPONSE_TIMEOUT_MS));
    }
#else
    (void)err;      /* ошибки остаются в очереди: SYSTem:ERRor? */
#endif
    return 0;
}

static scpi_result_t scpi_cb_control(scpi_t *context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val)
{
    (void)context; (void)ctrl; (void)val;
    return SCPI_RES_OK;
}

/* *RST: остановить все фоновые задачи. */
static scpi_result_t scpi_cb_reset(scpi_t *context)
{
    (void)context;
    iwp_job_stop_all(pdMS_TO_TICKS(500));
    return SCPI_RES_OK;
}

static scpi_interface_t s_interface = {
    .error   = scpi_cb_error,
    .write   = scpi_cb_write,
    .control = scpi_cb_control,
    .flush   = scpi_cb_flush,
    .reset   = scpi_cb_reset,
};

/* ---- Задача ------------------------------------------------------------- */

static void scpi_task(void *param)
{
    (void)param;
    char buf[64];

    for (;;) {
        size_t n = iwp_transport_read(buf, sizeof(buf), portMAX_DELAY);
        if (n > 0) {
            SCPI_Input(&s_context, buf, (int)n);
            response_flush();       /* на случай ответа без перевода строки */
        }
    }
}

bool iwp_scpi_port_init(void)
{
    SCPI_Init(&s_context, scpi_commands, &s_interface, scpi_units_def,
              SCPI_IDN_MANUFACTURER, SCPI_IDN_MODEL, iwp_sysinfo_uid(), SCPI_IDN_FIRMWARE,
              s_input_buffer, sizeof(s_input_buffer),
              s_error_queue, SCPI_ERROR_QUEUE_SIZE);

    return xTaskCreate(scpi_task, "scpi", STACK_SCPI, NULL, PRIO_SCPI, NULL) == pdPASS;
}
