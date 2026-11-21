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
