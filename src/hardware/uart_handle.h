#ifndef UART_HANDLE_H
#define UART_HANDLE_H

#include <FreeRTOS.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"

#include <task.h>
#include <stream_buffer.h>
#include <message_buffer.h>


extern StreamBufferHandle_t xRxUartStream;
extern StreamBufferHandle_t xTxUartStream;
void vUart_task(void *parameters) __attribute__((noreturn));
void vProcessing_task(void *pvParameters) __attribute__((noreturn));
void vDmaBuffering_task(void *pvParameters) __attribute__((noreturn));

void process_rx(uint16_t *buff, int size, MessageBufferHandle_t *mailbox);


#endif