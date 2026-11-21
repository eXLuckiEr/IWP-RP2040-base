#ifndef TRANSPORT_H
#define TRANSPORT_H

/* ===========================================================================
 *  transport — канал связи с хостом (сейчас USB-CDC через pico stdio).
 *
 *  Наружу торчат только два потока байт:
 *    RX: хост -> iwp_transport_read()  (читает SCPI-задача)
 *    TX: iwp_transport_write() -> хост (пишет кто угодно: SCPI-ответы, дамп данных)
 *
 *  iwp_transport_write() потокобезопасна: вызов атомарен относительно других
 *  вызовов (чужие данные не вклиниваются посреди вашего буфера). Если хост
 *  не подключён или не успевает читать, данные отбрасываются по таймауту —
 *  писатель никогда не зависает навсегда.
 *
 *  Чтобы заменить USB на настоящий UART, достаточно переписать backend_*()
 *  в transport.c.
 * ======================================================================== */

#include <stdbool.h>
#include <stddef.h>
#include <FreeRTOS.h>
#include <task.h>

bool   iwp_transport_init(void);

/* Записать len байт. Возвращает, сколько реально принято (< len при таймауте
 * или отсутствии хоста). */
size_t iwp_transport_write(const void *data, size_t len, TickType_t timeout);

/* Прочитать до max байт, ждать не дольше timeout. */
size_t iwp_transport_read(void *data, size_t max, TickType_t timeout);

bool   iwp_transport_connected(void);

#endif /* TRANSPORT_H */
