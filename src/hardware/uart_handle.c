#include "uart_handle.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"

#include <message_buffer.h>

#include "config/hardware_config.h"
#include <hardware/dma.h>

// ============================================================================
// Глобальные переменные
// ============================================================================
StreamBufferHandle_t xRxUartStream;
StreamBufferHandle_t xTxUartStream;
extern MessageBufferHandle_t xAdcMessageBuffer;

// ============================================================================
// Статические переменные для задачи обработки ADC
// ============================================================================
static uint16_t adc_samples_buffer[DMA_BUFFER_SIZE];
static uint16_t samples_count = 0;

// Статистика (для отладки)
static uint32_t total_samples = 0;
static uint32_t total_lines = 0;
static uint32_t tx_errors = 0;

extern uint16_t rx_buf_A[BUFFER_SIZE] __attribute__((aligned(2048)));
extern uint16_t rx_buf_B[BUFFER_SIZE] __attribute__((aligned(2048)));

extern int dma_tx;
extern int dma_rx_a;
extern int dma_rx_b;

extern volatile bool rx_a_ready;
extern volatile bool rx_b_ready;

// ============================================================================
// Задача UART: обработка приема и передачи данных через USB-CDC
// ============================================================================
void vUart_task(void *parameters)
{
    (void)parameters;

    uint8_t rx_buffer[32]; // Буфер для приема
    uint8_t tx_buffer[32]; // Буфер для передачи
    int rx_count = 0;
    int tx_count = 0;

    while (true)
    {
        if (stdio_usb_connected())
        {
            // ----- ПРИЕМ ДАННЫХ (USB -> Внутренний буфер) -----
            rx_count = stdio_get_until(rx_buffer, sizeof(rx_buffer), 0);
            if (rx_count > 0)
            {
                xStreamBufferSend(xRxUartStream, rx_buffer, rx_count, 0);
            }

            // ----- ПЕРЕДАЧА ДАННЫХ (Внутренний буфер -> USB) -----
            tx_count = xStreamBufferBytesAvailable(xTxUartStream);
            if (tx_count > sizeof(tx_buffer))
            {
                tx_count = sizeof(tx_buffer);
            }
            tx_count = xStreamBufferReceive(xTxUartStream, tx_buffer, tx_count, 0);
            if (tx_count > 0)
            {
                stdio_put_string(tx_buffer, tx_count, pdFALSE, pdFALSE);
            }

            vTaskDelay(pdMS_TO_TICKS(1));
        }
        else
        {
            // USB не подключен - ждем дольше
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void vDmaBuffering_task(void *pvParameters)
{
    (void)pvParameters;

    // ========== Главный цикл ==========
    while (true)
    {
        if (rx_a_ready)
        {
            process_rx(rx_buf_A, BUFFER_SIZE, &xAdcMessageBuffer);
            // Переконфигурируем для следующего цикла
            dma_channel_set_write_addr(dma_rx_a, rx_buf_A, false);
            dma_channel_set_trans_count(dma_rx_a, BUFFER_SIZE, false);
            rx_a_ready = false;
        }
        if (rx_b_ready)
        {
            process_rx(rx_buf_B, BUFFER_SIZE, &xAdcMessageBuffer);
            dma_channel_set_write_addr(dma_rx_b, rx_buf_B, false);
            dma_channel_set_trans_count(dma_rx_b, BUFFER_SIZE, false);
            rx_b_ready = false;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
        tight_loop_contents();
    }
}

void process_rx(uint16_t *buff, int size, MessageBufferHandle_t *mailbox)
{
    // uint16_t spaceAvailable = xMessageBufferSpaceAvailable(xAdcMessageBuffer);
    // if (spaceAvailable >= size)
    // {
    // size = xMessageBufferSend(xAdcMessageBuffer, buff, sizeof(uint16_t), 0);
    // size = xMessageBufferSend( *mailbox, buff, size * sizeof(uint16_t), 0 ) ;
    // }
    // else
    // {
    //     ;
    // }
    // uint16_t sample = 1024;
    if (xMessageBufferIsFull(*mailbox) == pdFALSE)
    {
        xMessageBufferSend(*mailbox, buff, 1 * sizeof(uint16_t), pdMS_TO_TICKS(0));
        // xMessageBufferSend(xAdcMessageBuffer, &sample, sizeof(uint16_t), pdMS_TO_TICKS(0));
    }
}

// ============================================================================
// Задача обработки ADC данных: форматирование и отправка в UART
// ============================================================================
void vProcessing_task(void *pvParameters)
{
    (void)pvParameters;

    char line_buffer[FORMATTED_LINE_MAX];
    TickType_t last_send_time = xTaskGetTickCount();
    const TickType_t max_idle_time = pdMS_TO_TICKS(MAX_IDLE_TIME_MS);

    while (true)
    {
        // ----- 1. ПРИЕМ ОТСЧЕТОВ ИЗ MESSAGE BUFFER -----
        uint16_t sample;
        while (xMessageBufferReceive(xAdcMessageBuffer, &sample, sizeof(uint16_t), 0) == sizeof(uint16_t))
        {
            if (samples_count < DMA_BUFFER_SIZE)
            {
                adc_samples_buffer[samples_count++] = sample;
                total_samples++;
            }
        }
        // while (!rx_b_ready && !rx_a_ready)
        // {
        //     if (rx_a_ready)
        //     {
        //         for (int i = 0; i < DMA_BUFFER_SIZE; i++)
        //         {
        //             adc_samples_buffer[i] = rx_buf_A[i];
        //             total_samples++;
        //         }

        //         // Переконфигурируем для следующего цикла
        //         dma_channel_set_write_addr(dma_rx_a, rx_buf_A, false);
        //         dma_channel_set_trans_count(dma_rx_a, BUFFER_SIZE, false);
        //         rx_a_ready = false;
        //     }
        //     if (rx_b_ready)
        //     {
        //         for (int i = 0; i < DMA_BUFFER_SIZE; i++)
        //         {
        //             adc_samples_buffer[i] = rx_buf_B[i];
        //             total_samples++;
        //         }
        //         dma_channel_set_write_addr(dma_rx_b, rx_buf_B, false);
        //         dma_channel_set_trans_count(dma_rx_b, BUFFER_SIZE, false);
        //         rx_b_ready = false;
        //     }
        // }

        // ----- 2. ПРОВЕРКА УСЛОВИЙ ДЛЯ ОТПРАВКИ -----
        bool need_to_send = false;

        // Условие 1: Есть хотя бы один отсчет
        if (samples_count >= 1)
        {
            need_to_send = true;
        }

        // Условие 2: Истек таймаут ожидания
        if (samples_count > 0 && (xTaskGetTickCount() - last_send_time) >= max_idle_time)
        {
            need_to_send = true;
        }

        // ----- 3. ФОРМАТИРОВАНИЕ И ОТПРАВКА -----
        if (need_to_send && samples_count > 0)
        {

            // Отправляем каждую строку отдельно с проверкой свободного места
            for (uint16_t i = 0; i < samples_count; i++)
            {

                // Форматируем одну строку
                int line_len = snprintf(line_buffer, sizeof(line_buffer),
                                        ">Val:%u\r\n", adc_samples_buffer[i]);

                if (line_len > 0 && line_len < FORMATTED_LINE_MAX)
                {

                    // Проверяем, достаточно ли места в TX буфере для целой строки
                    size_t free_space = xStreamBufferSpacesAvailable(xTxUartStream);

                    if (free_space >= (size_t)line_len)
                    {
                        // Места достаточно - отправляем целую строку
                        size_t sent = xStreamBufferSend(xTxUartStream, line_buffer, line_len, 0);
                        if (sent == (size_t)line_len)
                        {
                            total_lines++;
                        }
                        else
                        {
                            tx_errors++;
                        }
                    }
                    else
                    {
                        // Нет места - откладываем отправку оставшихся строк
                        tx_errors++;

                        // Сдвигаем неотправленные отсчеты в начало буфера
                        if (i < samples_count)
                        {
                            memmove(adc_samples_buffer,
                                    adc_samples_buffer + i,
                                    (samples_count - i) * sizeof(uint16_t));
                            samples_count -= i;
                        }
                        goto wait_and_retry; // Выходим из цикла отправки
                    }
                }
            }

            // Все строки успешно отправлены - сбрасываем счетчики
            samples_count = 0;
            last_send_time = xTaskGetTickCount();
        }

    wait_and_retry:
        vTaskDelay(pdMS_TO_TICKS(PROCESSING_DELAY_MS));
    }
}

// ============================================================================
// Опционально: функция для получения статистики (для отладки)
// ============================================================================
void vProcessing_GetStats(uint32_t *samples, uint32_t *lines, uint32_t *errors)
{
    if (samples != NULL)
        *samples = total_samples;
    if (lines != NULL)
        *lines = total_lines;
    if (errors != NULL)
        *errors = tx_errors;
}

// ============================================================================
// Опционально: функция для сброса статистики
// ============================================================================
void vProcessing_ResetStats(void)
{
    total_samples = 0;
    total_lines = 0;
    tx_errors = 0;
}
