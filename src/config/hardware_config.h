#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// SPI configuration
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS 17
#define PIN_SCK 18
#define PIN_MOSI 19

// ADC configuration
#define ADC_SAMPLE_RATE 100
#define ADC_BYTES_PER_SAMPLE 2
#define DMA_BUFFER_SIZE 256
#define DMA_BUFFER_BYTES (DMA_BUFFER_SIZE * ADC_BYTES_PER_SAMPLE)
#define MESSAGE_BUFFER_SIZE 4096

// UART configuration
#define UART_QUEUE_LENGTH 128

// Task priorities
#define PRIORITY_ADC_DMA 3
#define PRIORITY_PROCESSING 2
#define PRIORITY_SCPI 2
#define PRIORITY_UART 1

// Stack sizes
#define STACK_ADC_DMA (configMINIMAL_STACK_SIZE + 256)
#define STACK_PROCESSING (configMINIMAL_STACK_SIZE + 512)
#define STACK_SCPI (configMINIMAL_STACK_SIZE + 256)
#define STACK_UART (configMINIMAL_STACK_SIZE + 256)

// ADC Processing configuration
#define FORMATTED_LINE_MAX      20      // Максимальная длина одной строки ">Val:65535\r\n"
#define MAX_IDLE_TIME_MS        50      // Максимальное время простоя перед отправкой (мс)
#define PROCESSING_DELAY_MS     2       // Задержка в цикле обработки (мс)

// UART Buffers
#define UART_RX_BUFFER_SIZE     256     // Размер RX буфера
#define UART_TX_BUFFER_SIZE     4096    // Размер TX буфера (увеличен для предотвращения переполнения)

#endif
