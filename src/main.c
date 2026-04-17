/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <message_buffer.h>

/* Standard includes. */
#include <stdio.h>
#include <stdint.h>

/* */
#include <timers.h>
#include <hardware/spi.h>
#include "pico/stdlib.h"

/* Project includes. */
#include "config/hardware_config.h"
#include "hardware/uart_handle.h"
#include "scpi-extensions/scpi-def.h"
#include "hardware/dma_adc.h"

struct repeating_timer led_timer;
struct repeating_timer adc_timer;

TaskHandle_t vUart_handle = NULL;
TaskHandle_t vSCPI_handle = NULL;
TaskHandle_t vProcessing_handle = NULL;

MessageBufferHandle_t xAdcMessageBuffer;

bool led_timer_callback(__unused struct repeating_timer *rt);
bool adc_timer_callback(__unused struct repeating_timer *rt);

void hardware_init(void);

int main()
{
    stdio_init_all();
    hardware_init();

    xAdcMessageBuffer = xMessageBufferCreate(MESSAGE_BUFFER_SIZE);
    configASSERT(xAdcMessageBuffer != NULL);

    xRxUartStream = xStreamBufferCreate(UART_QUEUE_LENGTH, sizeof(uint8_t));
    configASSERT(xRxUartStream != NULL);

    xTxUartStream = xStreamBufferCreate(UART_QUEUE_LENGTH, sizeof(uint8_t));
    configASSERT(xTxUartStream != NULL);

    // Create tasks
    xTaskCreate(vSCPI_task, "vSCPI_task", STACK_SCPI, NULL, 2, &vSCPI_handle);
    xTaskCreate(vUart_task, "vUart_task", STACK_UART, NULL, 2, &vUart_handle);
    // xTaskCreate(vADC_DMA_task, "ADC_DMA", configMINIMAL_STACK_SIZE + 256, NULL, 3, &ADC_DmaHandle);
    xTaskCreate(vProcessing_task, "Processing", configMINIMAL_STACK_SIZE + 512, NULL, 2, &vProcessing_handle);

    // Start
    vTaskStartScheduler();
    while (true)
    {
        /* Should not reach here. */
    }
}

void hardware_init(void)
{

    /* GPIO ------------------------------------------- */
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    /* SPI ------------------------------------------- */
    dma_adc_init();

    /* TIMERS ------------------------------------------- */
    add_repeating_timer_ms(500, led_timer_callback, NULL, &led_timer);
    add_repeating_timer_us(250, adc_timer_callback, NULL, &adc_timer);
    // add_repeating_timer_us(-1000000 / ADC_SAMPLE_RATE, adc_timer_callback, NULL, &adc_timer);

    /* I2C ------------------------------------------- */
    // This example will use I2C0 on the default SDA and SCL pins (GP4, GP5 on a Pico)
    // i2c_init(I2C_PORT, 100 * 1000);
    // gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    // gpio_set_function(I2C_SCLK, GPIO_FUNC_I2C);
    // gpio_pull_up(I2C_SDA);
    // gpio_pull_up(I2C_SCLK);

    /* Display ------------------------------------------- */
    // ssd1309_advance_init(SSD1309_INTERFACE_SPI, SSD1309_ADDR_SA0_0);
}

bool led_timer_callback(__unused struct repeating_timer *rt)
{
    bool state = gpio_get(PICO_DEFAULT_LED_PIN);
    gpio_put(PICO_DEFAULT_LED_PIN, state ? false : true);
    return true;
}

bool adc_timer_callback(struct repeating_timer *rt)
{
    uint16_t sample;
    spi_read16_blocking(SPI_PORT, (uint16_t)(0xffff), &sample, 1);
    if(xMessageBufferIsFull(xAdcMessageBuffer) == pdFALSE){
        xMessageBufferSend(xAdcMessageBuffer, &sample, sizeof(uint16_t), pdMS_TO_TICKS(0));
    }
    return true;
}
