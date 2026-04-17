#include "hardware/dma_adc.h"

// #include <message_buffer.h>
// #include <stream_buffer.h>

#include "config/hardware_config.h"
// #include <hardware/dma.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>


// static int dma_rx_channel;
// static int dma_tx_channel;
// static uint8_t rx_buffer[DMA_BUFFER_BYTES];
// static uint8_t tx_dummy_buffer[DMA_BUFFER_BYTES];
// static volatile bool dma_complete = false;
// static void (*irq_callback)(void) = NULL;

void dma_adc_init(void)
{

    /* SPI ------------------------------------------- */
    // Enable SPI 0 at 1 MHz and connect to GPIOs
    spi_init(SPI_PORT, 1000000u);
    spi_set_format(SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS, GPIO_FUNC_SPI);

    // hw_set_bits(&spi_get_hw(SPI_PORT)->dmacr, SPI_SSPDMACR_RXDMAE_BITS | SPI_SSPDMACR_TXDMAE_BITS);
};

// void dma_init(void){

//     /* DMA ------------------------------------------- */
//     dma_rx_channel = dma_claim_unused_channel(true);

//     dma_channel_config rx_config = dma_channel_get_default_config(dma_rx_channel);
//     channel_config_set_transfer_data_size(&rx_config, DMA_SIZE_16);
//     channel_config_set_read_increment(&rx_config, false);               // Всегда из регистра данных SPI
//     channel_config_set_write_increment(&rx_config, true);               // В буфер последовательно
//     channel_config_set_dreq(&rx_config, spi_get_dreq(SPI_PORT, false)); // DREQ_SPI0_RX

//     dma_channel_configure(
//         dma_rx_channel,
//         &rx_config,
//         rx_buffer,                 // Куда пишем
//         &spi_get_hw(SPI_PORT)->dr, // Откуда читаем (регистр данных SPI)
//         DMA_BUFFER_BYTES,          // Сколько байт
//         false                      // Не стартуем автоматически
//     );

//     memset(tx_dummy_buffer, 0, DMA_BUFFER_BYTES);

//     dma_tx_channel = dma_claim_unused_channel(true);
//     dma_channel_config tx_config = dma_channel_get_default_config(dma_tx_channel);
//     channel_config_set_transfer_data_size(&tx_config, DMA_SIZE_16);
//     channel_config_set_read_increment(&tx_config, true);
//     channel_config_set_write_increment(&tx_config, false);
//     channel_config_set_dreq(&tx_config, spi_get_dreq(SPI_PORT, true)); // TX DREQ

//     dma_channel_configure(
//         dma_tx_channel,
//         &tx_config,
//         &spi_get_hw(SPI_PORT)->dr,  // Пишем в SPI
//         tx_dummy_buffer,            // Отправляем нули
//         DMA_BUFFER_BYTES,
//         false
//     );

//     // Настройка прерывания по завершению DMA
//     dma_channel_set_irq0_enabled(dma_rx_channel, true);
//     irq_set_exclusive_handler(DMA_IRQ_0, dma_rx_irq_handler);
//     irq_set_enabled(DMA_IRQ_0, true);
// }

// void dma_rx_irq_handler(void)
// {
//     if (dma_hw->ints0 & (1u << dma_rx_channel))
//     {
//         dma_hw->ints0 = 1u << dma_rx_channel; // Сбрасываем флаг
//         dma_complete = true;

//         // Уведомляем задачу о завершении DMA
//         BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//         vTaskNotifyGiveFromISR(adc_task_handle, &xHigherPriorityTaskWoken);
//         portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
//     }
// }

// void start_dma_read(void)
// {
//     dma_complete = false;
//     dma_channel_start(dma_tx_channel);  // Отправляем нули (генерируем SCK)
//     dma_channel_start(dma_rx_channel);  // Принимаем данные от АЦП
// }

// void vADC_DMA_task(void *pvParameters) {
//     // Сохраняем хендл текущей задачи для уведомлений
//     adc_task_handle = xTaskGetCurrentTaskHandle();

//     for (;;) {
//         // Ожидаем завершения DMA
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

//         // Проверяем, что DMA действительно завершился
//         if (dma_complete) {
//             dma_complete = false;

//             // Отправляем данные в Message Buffer
//             size_t bytes_sent = xMessageBufferSend(
//                 xAdcMessageBuffer,
//                 rx_buffer,
//                 DMA_BUFFER_BYTES,
//                 pdMS_TO_TICKS(10)  // Ждём до 10 мс, если буфер полон
//             );

//             if (bytes_sent != DMA_BUFFER_BYTES) {
//                 // Буфер переполнен - считаем ошибку
//                 static uint32_t overflow_count = 0;
//                 if (++overflow_count % 100 == 0) {
//                     printf("Warning: Message buffer overflow (%d)\n", overflow_count);
//                 }
//             }

//             // Переконфигурируем DMA для следующего буфера
//             dma_channel_set_write_addr(dma_rx_channel, rx_buffer, false);
//         }
//     }
// }

// void vProcessing_task(void *pvParameters)
// {
//     uint16_t samples[DMA_BUFFER_SIZE];
//     size_t bytes_received;

//     uint32_t total_samples = 0;
//     TickType_t last_print = xTaskGetTickCount();

//     for (;;)
//     {
//         bytes_received = xMessageBufferReceive(
//             xAdcMessageBuffer,
//             samples,
//             DMA_BUFFER_BYTES,
//             portMAX_DELAY);

//         if (bytes_received > 0)
//         {
//             int num_samples = bytes_received / sizeof(uint16_t);
//             total_samples += num_samples;

//             for (int i = 0; i < num_samples; i++)
//             {
//                 // Пример: проверка значения АЦП
//                 uint16_t raw_value = samples[i];

//                 // Конвертация в напряжение (пример для АЦП с Vref=3.3V)
//                 float voltage = (raw_value * 3.3f) / 65535.0f;

//                 // Ваша обработка: фильтрация, FFT, анализ и т.д.
//                 process_adc_sample(raw_value, voltage);
//             }

//             // Печатаем статистику каждую секунду
//             if ((xTaskGetTickCount() - last_print) >= pdMS_TO_TICKS(1000))
//             {
//                 printf("Processed %lu samples (%.1f Hz)\n",
//                 total_samples,
//                 total_samples / ((xTaskGetTickCount() - last_print) / configTICK_RATE_HZ));
//                 total_samples = 0;
//                 last_print = xTaskGetTickCount();
//             }
//         }
//     }
// }


// // ==================== Пример обработки сэмпла ====================
// void process_adc_sample(uint16_t raw_value, float voltage)
// {
//     static float filter_state = 0;

//     // Простой low-pass фильтр
//     float alpha = 0.1f;
//     filter_state = filter_state * (1.0f - alpha) + voltage * alpha;

//     // Здесь можно сохранять в циклический буфер, отправлять по UART и т.д.
//     printf("ADC: %5d -> %.3f V (filtered: %.3f)\n", raw_value, voltage, filter_state);
// }
