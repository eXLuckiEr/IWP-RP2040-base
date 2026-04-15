#include "uart_handle.h"

StreamBufferHandle_t xRxUartStream;
StreamBufferHandle_t xTxUartStream;

void vUart_task(void *parameters)
{
    (void)parameters;

    uint8_t rx_buffer[32];  // Буфер для приема
    uint8_t tx_buffer[32];  // Буфер для передачи
    int rx_count = 0;
    int tx_count = 0;

    while (true)
    {
        if(stdio_usb_connected()) {
            rx_count = stdio_get_until(rx_buffer, sizeof(rx_buffer), 0);
            if(rx_count > 0) {
                xStreamBufferSend(xRxUartStream, rx_buffer, rx_count, 0);
            }

            tx_count = xStreamBufferBytesAvailable(xTxUartStream);
            if(tx_count > sizeof(tx_buffer)) {
                tx_count = sizeof(tx_buffer);
            }
            tx_count = xStreamBufferReceive(xTxUartStream, tx_buffer, tx_count, 0);
            if(tx_count > 0) {
                stdio_put_string(tx_buffer, tx_count, pdFALSE, pdFALSE);
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        // print_spi_buffer(&xAdcMessageBuffer);
    }
}

void print_spi_buffer(StreamBufferHandle_t *st_buffer)
{

    uint16_t adc_data[16];

    // if (xMessageBufferIsEmpty(*st_buffer) == pdFALSE)
    // {
    //     xMessageBufferReceive(*st_buffer, adc_data, 2, pdMS_TO_TICKS(0));
    //     printf(">Val:%d\r\n", adc_data[0]);
    // }
}
