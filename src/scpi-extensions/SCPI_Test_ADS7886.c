#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scpi/scpi.h"
#include "scpi-def.h"

#include <FreeRTOS.h>

#include "config/hardware_config.h"
#include <hardware/dma.h>

extern uint16_t rx_buf_A[BUFFER_SIZE] __attribute__((aligned(2048)));
extern uint16_t rx_buf_B[BUFFER_SIZE] __attribute__((aligned(2048)));

extern int dma_tx;
extern int dma_rx_a;
extern int dma_rx_b;

extern volatile bool rx_a_ready;
extern volatile bool rx_b_ready;

scpi_result_t SCPI_adc_dump_n(scpi_t *context)
{
    (void)context;
    uint32_t param1;

    if (!SCPI_ParamUInt32(context, &param1, FALSE))
    {
        return SCPI_RES_ERR;
    }

    if (param1 == 0)
    {
        param1 = 1;
    }

    // ========== Старт обоих одновременно ==========
    dma_channel_start(dma_rx_a); // RX первый — чтобы не пропустить данные
    dma_channel_start(dma_tx);   // TX запускает тактирование
  
    printf("= = = = = = = = = = = = = = = = = = = = = = \n");
    printf("DMA started!\n");

    return SCPI_RES_OK;
}
