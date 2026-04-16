#include "uart_handle.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include <message_buffer.h>

StreamBufferHandle_t xRxUartStream;
StreamBufferHandle_t xTxUartStream;
extern MessageBufferHandle_t xAdcMessageBuffer;

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
            rx_count = stdio_get_until(rx_buffer, sizeof(rx_buffer), 0);
            if (rx_count > 0)
            {
                xStreamBufferSend(xRxUartStream, rx_buffer, rx_count, 0);
            }

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
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void vProcessing_task(void *pvParameters)
{
    (void)pvParameters;

    uint16_t sample;
    char buffer[32];

    while (true)
    {
        if (xMessageBufferReceive(xAdcMessageBuffer, &sample, sizeof(uint16_t), pdMS_TO_TICKS(100)) == sizeof(uint16_t))
        {
            snprintf(buffer, sizeof(buffer), ">Val:%u\r\n", sample);
            xStreamBufferSend(xTxUartStream, buffer, strlen(buffer), 0);
        }
    }
}
