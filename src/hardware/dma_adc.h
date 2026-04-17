#ifndef DMA_ADC_H
#define DMA_ADC_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// #include <message_buffer.h>
// #include <task.h>

// #include "pico/stdlib.h"
// #include "config/hardware_config.h"


void dma_adc_init(void);

// static int dma_rx_channel;
// static uint8_t rx_buffer[DMA_BUFFER_BYTES];

// static int dma_tx_channel;
// static uint8_t tx_dummy_buffer[DMA_BUFFER_BYTES];

// static volatile bool dma_complete = false;
// static TaskHandle_t adc_task_handle = NULL;

// void vADC_DMA_task(void *pvParameters) __attribute__((noreturn));


// void dma_rx_irq_handler(void);
// void start_dma_read(void);
// void process_adc_sample(uint16_t raw_value, float voltage);

// void dma_adc_start_read(void);
// bool dma_adc_is_complete(void);
// void dma_adc_reset_complete_flag(void);
// uint8_t* dma_adc_get_buffer(void);
// size_t dma_adc_get_buffer_bytes(void);
// void dma_adc_irq_handler(void);
// void dma_adc_register_callback(void (*callback)(void));

#endif
