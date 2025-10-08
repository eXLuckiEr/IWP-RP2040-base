/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include <pico/multicore.h>
#include <task.h>

#include <stream_buffer.h>
#include <timers.h>
#include <semphr.h>


/* Standard includes. */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "pico/stdlib.h"

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#include "hardware/spi.h"

/* SCPI includes. */
#include "scpi-def.h"

/* SSD1309 includes. */

#include "driver_ssd1309_interface.h"
#include "driver_ssd1309_advance.h"

#include "portmacro.h"

TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t scpiTaskHandle = NULL;
TaskHandle_t spiTaskHandle = NULL;
TaskHandle_t uartTaskHandle = NULL;

#define ADC_QUEUE_LENGTH 128
#define UART_QUEUE_LENGTH 128
StreamBufferHandle_t SpiStream;
StreamBufferHandle_t UartStream;

void spi_task ( void * parameters ) __attribute__( ( noreturn ) );
void led_task ( void * parameters ) __attribute__( ( noreturn ) );
void scpi_task( void * parameters ) __attribute__( ( noreturn ) );
void uart_task( void * parameters ) __attribute__( ( noreturn ) );

void hardware_init(void);

int main()
{
    stdio_init_all();
    hardware_init();

    SpiStream = xStreamBufferCreate(ADC_QUEUE_LENGTH, sizeof(uint16_t));
    UartStream = xStreamBufferCreate(UART_QUEUE_LENGTH, sizeof(uint8_t));

    ssd1309_advance_init(SSD1309_INTERFACE_SPI, SSD1309_ADDR_SA0_0);

    // Create tasks
    xTaskCreate(led_task, "LED_task", 256, NULL, 2, &ledTaskHandle);
    // xTaskCreate(spi_task, "SPI_task", 256, NULL, 2, &spiTaskHandle);
    xTaskCreate(scpi_task, "SCPItask", 256, NULL, 2, &scpiTaskHandle);
    xTaskCreate(uart_task, "Uart_task", 256, NULL, 2, &uartTaskHandle);

    // Start
    vTaskStartScheduler();
    while (true) {
        /* Should not reach here. */
    }
}

void hardware_init(void){

    /* GPIO ------------------------------------------- */
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    /* I2C ------------------------------------------- */
    // This example will use I2C0 on the default SDA and SCL pins (GP4, GP5 on a Pico)
    // i2c_init(I2C_PORT, 100 * 1000);
    // gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    // gpio_set_function(I2C_SCLK, GPIO_FUNC_I2C);
    // gpio_pull_up(I2C_SDA);
    // gpio_pull_up(I2C_SCLK);

    /* SPI ------------------------------------------- */
    // Enable SPI 0 at 1 MHz and connect to GPIOs
    // spi_init(SPI_PORT, 1000000u);
    // spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    // spi_set_format(SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    // gpio_set_function(PIN_CS,   GPIO_FUNC_SPI);
    // gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    // gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
}

void spi_task( void * parameters )
{
    /* Unused parameters. */
    ( void ) parameters;

    uint16_t adc_value = 0;
    
    printf("SPI master example\n");
    while (true) {
        spi_read16_blocking(SPI_PORT, (uint16_t)(0xffff), &adc_value, 1);
        xStreamBufferSend(SpiStream, &adc_value, sizeof(uint16_t), pdMS_TO_TICKS(1));
        taskYIELD();
        // vTaskDelay( 10 ); /* delay 100 ticks */
    }
}

void led_task( void * parameters )
{   
    ( void ) parameters;

    while (true) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        vTaskDelay(250);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        vTaskDelay(250);
    }
}

void scpi_task( void * parameters )
{   
    ( void ) parameters;

    SCPI_Init(&scpi_context,
            scpi_commands,
            &scpi_interface,
            scpi_units_def,
            SCPI_IDN1, SCPI_IDN2, SCPI_IDN3, SCPI_IDN4,
            scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
            scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);

    char buffer[64];
    size_t received = 0;
    while (true) {
        if(xStreamBufferIsFull(UartStream) == pdFALSE){
            received = xStreamBufferReceive(UartStream, buffer, sizeof(buffer), pdMS_TO_TICKS(10));
        } else {
            vTaskDelay(50);
        }
        if(received > 0){
            SCPI_Input(&scpi_context, buffer, received);
        }
    }
}

void uart_task( void * parameters )
{   
    ( void ) parameters;

    uint8_t data = 0;
    while (true) {
        data = getchar();
        xStreamBufferSend(UartStream, &data, sizeof(uint8_t), pdMS_TO_TICKS(0));
        taskYIELD();
    }
}

#if ( configCHECK_FOR_STACK_OVERFLOW > 0 )

    void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                        char * pcTaskName )
    {
        /* Check pcTaskName for the name of the offending task,
         * or pxCurrentTCB if pcTaskName has itself been corrupted. */
        ( void ) xTask;
        ( void ) pcTaskName;
    }

#endif /* #if ( configCHECK_FOR_STACK_OVERFLOW > 0 ) */
