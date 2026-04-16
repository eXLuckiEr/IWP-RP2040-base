#ifndef UART_HANDLE_H
#define UART_HANDLE_H

#include <FreeRTOS.h>

#include <task.h>
#include <stream_buffer.h>

extern StreamBufferHandle_t xRxUartStream;
extern StreamBufferHandle_t xTxUartStream;
void vUart_task(void *parameters) __attribute__((noreturn));
void vProcessing_task(void *pvParameters) __attribute__((noreturn));

#endif