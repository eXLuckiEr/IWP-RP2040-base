#include "transport.h"

#include <FreeRTOS.h>
#include <semphr.h>
#include <stream_buffer.h>
#include <task.h>

#include "pico/stdlib.h"
#include "pico/stdio_usb.h"

#include "config/board_config.h"
#include "core/job.h"

static StreamBufferHandle_t s_rx;
static StreamBufferHandle_t s_tx;
static SemaphoreHandle_t    s_tx_mutex;
static TaskHandle_t         s_task;

/* ---- backend: USB-CDC --------------------------------------------------- */

static bool backend_connected(void)
{
    return stdio_usb_connected();
}

/* Забрать то, что уже пришло по USB (без ожидания). */
static int backend_read(uint8_t *buf, int max)
{
    int n = 0;
    while (n < max) {
        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT) {
            break;
        }
        buf[n++] = (uint8_t)c;
    }
    return n;
}

static void backend_write(const uint8_t *buf, size_t len)
{
    stdio_put_string((const char *)buf, (int)len, false, false);
}

static void backend_flush(void)
{
    stdio_flush();
}

/* Вызывается из прерывания при поступлении байтов по USB: будим задачу. */
static void chars_available_cb(void *param)
{
    (void)param;
    if (s_task != NULL) {
        BaseType_t woken = pdFALSE;
        vTaskNotifyGiveFromISR(s_task, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

/* ---- задача -------------------------------------------------------------- */

static void iwp_transport_task(void *param)
{
    (void)param;
    uint8_t buf[TRANSPORT_TX_CHUNK_BYTES];
    bool was_connected = false;

    stdio_set_chars_available_callback(chars_available_cb, NULL);

    for (;;) {
        /* Спим до события (новые RX/TX-данные) либо до таймаута: на таймауте
         * проверяем состояние подключения. */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20));

        bool connected = backend_connected();
        if (was_connected && !connected) {
            /* Хост пропал: никто не читает дамп — останавливаем все job. */
            iwp_job_request_stop_all();
        }
        was_connected = connected;

        if (!connected) {
            /* Выбрасываем накопленное, чтобы писатели не упирались в
             * переполненный буфер. */
            xStreamBufferReset(s_tx);
            continue;
        }

        /* ---- RX: USB -> поток. Читаем только если есть место: иначе
         * оставляем байты в USB, и хост получает естественный backpressure. */
        for (;;) {
            size_t room = xStreamBufferSpacesAvailable(s_rx);
            if (room == 0) {
                break;
            }
            uint8_t rx[32];
            int n = backend_read(rx, room < sizeof(rx) ? (int)room : (int)sizeof(rx));
            if (n <= 0) {
                break;
            }
            /* Break-байт (Ctrl-C) — аварийная остановка в обход парсера. */
            int keep = 0;
            for (int i = 0; i < n; i++) {
                if (rx[i] == TRANSPORT_BREAK_BYTE) {
                    iwp_job_request_stop_all();
                } else {
                    rx[keep++] = rx[i];
                }
            }
            if (keep > 0) {
                xStreamBufferSend(s_rx, rx, (size_t)keep, 0);
            }
        }

        /* ---- TX: поток -> USB ---- */
        bool wrote = false;
        size_t n;
        while ((n = xStreamBufferReceive(s_tx, buf, sizeof(buf), 0)) > 0) {
            backend_write(buf, n);
            wrote = true;
        }
        if (wrote) {
            backend_flush();
        }
    }
}

/* ---- публичный API ------------------------------------------------------ */

bool iwp_transport_init(void)
{
    s_rx = xStreamBufferCreate(TRANSPORT_RX_BUFFER_BYTES, 1);
    s_tx = xStreamBufferCreate(TRANSPORT_TX_BUFFER_BYTES, 1);
    s_tx_mutex = xSemaphoreCreateMutex();
    if (s_rx == NULL || s_tx == NULL || s_tx_mutex == NULL) {
        return false;
    }
    return xTaskCreate(iwp_transport_task, "transport", STACK_TRANSPORT, NULL,
                       PRIO_TRANSPORT, &s_task) == pdPASS;
}

bool iwp_transport_connected(void)
{
    return backend_connected();
}

size_t iwp_transport_write(const void *data, size_t len, TickType_t timeout)
{
    if (data == NULL || len == 0 || !backend_connected()) {
        return 0;
    }

    TickType_t start = xTaskGetTickCount();
    if (xSemaphoreTake(s_tx_mutex, timeout) != pdTRUE) {
        return 0;
    }

    const uint8_t *p = (const uint8_t *)data;
    size_t done = 0;
    const size_t slice_max = TRANSPORT_TX_BUFFER_BYTES / 2;

    while (done < len) {
        TickType_t used = xTaskGetTickCount() - start;
        TickType_t left = (used >= timeout) ? 0 : (timeout - used);
        size_t slice = len - done;
        if (slice > slice_max) {
            slice = slice_max;
        }
        size_t sent = xStreamBufferSend(s_tx, p + done, slice, left);
        xTaskNotifyGive(s_task);
        if (sent == 0) {
            break;      /* таймаут: хост не успевает */
        }
        done += sent;
    }

    xSemaphoreGive(s_tx_mutex);
    return done;
}

size_t iwp_transport_read(void *data, size_t max, TickType_t timeout)
{
    size_t n = xStreamBufferReceive(s_rx, data, max, timeout);
    if (n > 0) {
        xTaskNotifyGive(s_task);   /* в RX освободилось место — забрать из USB */
    }
    return n;
}
