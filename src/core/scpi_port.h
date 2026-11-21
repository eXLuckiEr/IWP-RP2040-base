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

#ifndef SCPI_PORT_H
#define SCPI_PORT_H

/* ===========================================================================
 *  scpi_port — связывает libscpi с transport и FreeRTOS.
 *
 *  Правила для SCPI-колбэков (важно для неблокирующей работы):
 *    1. Колбэк выполняется в SCPI-задаче. Пока он работает, новые команды не
 *       разбираются. Поэтому колбэк должен возвращаться за миллисекунды.
 *    2. Долгую работу выносите в job (core/job.h): колбэк делает
 *       iwp_job_start()/iwp_job_stop() и сразу возвращается.
 *    3. Ответы пишите через SCPI_Result*(). Данные «в сторону» (дамп потока)
 *       отправляйте из job через iwp_transport_write().
 * ======================================================================== */

#include <stdbool.h>
#include "scpi/scpi.h"

extern const scpi_command_t scpi_commands[];

/* Создаёт SCPI-задачу. Вызывать после iwp_transport_init(). */
bool iwp_scpi_port_init(void);

#endif /* SCPI_PORT_H */
