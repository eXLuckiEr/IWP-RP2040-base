#include "hardware/dma_adc.h"
#include <FreeRTOS.h>

#include "config/hardware_config.h"
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <hardware/dma.h>
#include "hardware/irq.h"

uint16_t rx_buf_A[BUFFER_SIZE] __attribute__((aligned(2048)));
uint16_t rx_buf_B[BUFFER_SIZE] __attribute__((aligned(2048)));

int dma_tx;
int dma_rx_a;
int dma_rx_b;

volatile bool rx_a_ready = false;
volatile bool rx_b_ready = false;

static void dma_irq_handler(void)
{
    if (dma_hw->ints0 & (1u << dma_rx_a))
    {
        dma_hw->ints0 = (1u << dma_rx_a);
        rx_a_ready = true;
    }
    if (dma_hw->ints0 & (1u << dma_rx_b))
    {
        dma_hw->ints0 = (1u << dma_rx_b);
        rx_b_ready = true;
    }
}

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

    /* DMA --------------------------------------------- */

    static uint16_t txbuf[TEST_SIZE];
    for (uint i = 0; i < TEST_SIZE; ++i)
    {
        txbuf[i] = i + 5;
    }

    // ========== DMA Timer: 44100 Гц ==========
    dma_hw->timer[0] = (1u << 16) | 3000u;

    // ========== TX DMA — без изменений ==========
    dma_tx = dma_claim_unused_channel(true);
    dma_channel_config tx_cfg = dma_channel_get_default_config(dma_tx);

    channel_config_set_transfer_data_size(&tx_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&tx_cfg, true);
    channel_config_set_write_increment(&tx_cfg, false);
    channel_config_set_dreq(&tx_cfg, DREQ_DMA_TIMER0); // pacing 44100
    channel_config_set_chain_to(&tx_cfg, dma_tx);      // loop
    channel_config_set_ring(&tx_cfg, false, 11);       // read ring 2048 байт

    dma_channel_configure(dma_tx, &tx_cfg,
                          &spi_get_hw(SPI_PORT)->dr,
                          &txbuf[0],
                          0xFFFFFFFF,
                          false); // пока не стартуем

    // ========== RX DMA — ping-pong ==========
    dma_rx_a = dma_claim_unused_channel(true);
    dma_rx_b = dma_claim_unused_channel(true);

    // RX канал A
    dma_channel_config rx_cfg_a = dma_channel_get_default_config(dma_rx_a);
    channel_config_set_transfer_data_size(&rx_cfg_a, DMA_SIZE_16);
    channel_config_set_read_increment(&rx_cfg_a, false);               // читаем из SPI RX — фикс. адрес
    channel_config_set_write_increment(&rx_cfg_a, true);               // пишем в буфер — инкремент
    channel_config_set_dreq(&rx_cfg_a, spi_get_dreq(SPI_PORT, false)); // DREQ от SPI RX
    channel_config_set_chain_to(&rx_cfg_a, dma_rx_b);                  // после A → запуск B

    dma_channel_configure(dma_rx_a, &rx_cfg_a,
                          rx_buf_A,                  // куда: буфер A
                          &spi_get_hw(SPI_PORT)->dr, // откуда: SPI RX
                          BUFFER_SIZE,               // 1024 слова
                          false);

    // RX канал B
    dma_channel_config rx_cfg_b = dma_channel_get_default_config(dma_rx_b);
    channel_config_set_transfer_data_size(&rx_cfg_b, DMA_SIZE_16);
    channel_config_set_read_increment(&rx_cfg_b, false);
    channel_config_set_write_increment(&rx_cfg_b, true);
    channel_config_set_dreq(&rx_cfg_b, spi_get_dreq(SPI_PORT, false));
    channel_config_set_chain_to(&rx_cfg_b, dma_rx_a); // после B → запуск A

    dma_channel_configure(dma_rx_b, &rx_cfg_b,
                          rx_buf_B,
                          &spi_get_hw(SPI_PORT)->dr,
                          BUFFER_SIZE,
                          false);

    // RX прерывания
    dma_channel_set_irq0_enabled(dma_rx_a, true);
    dma_channel_set_irq0_enabled(dma_rx_b, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);

};

