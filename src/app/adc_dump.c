/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "adc_dump.h"

#include <stdint.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/spi.h"

#include "config/board_config.h"
#include "core/job.h"
#include "core/transport.h"

/* ---- Константы ----------------------------------------------------------- */

#define ADC_N                   ADC_DMA_BUF_SAMPLES
#define EV_BUF(i)           (1u << (i))
#define PACKET_MAGIC_0      0xA5
#define PACKET_MAGIC_1      0x5A
#define BIN_HEADER_BYTES    6
#define ASCII_LINE_MAX      (sizeof(ADC_ASCII_PREFIX) - 1 + 5 + 2)  /* prefix + "65535" + CRLF */
#define TX_SCRATCH_BYTES    ((ADC_N * ASCII_LINE_MAX) > (BIN_HEADER_BYTES + ADC_N * 2) ? \
                             (ADC_N * ASCII_LINE_MAX) : (BIN_HEADER_BYTES + ADC_N * 2))
#define SEND_TIMEOUT_MS     100
#define STOP_TIMEOUT_MS     500

typedef enum { FMT_ASCII = 0, FMT_BINARY = 1 } adc_format_t;

static const scpi_choice_def_t s_format_choices[] = {
    { "ASCii", FMT_ASCII  },
    { "BINary", FMT_BINARY },
    SCPI_CHOICE_LIST_END
};

/* ---- Состояние ----------------------------------------------------------- */

static iwp_job_t s_job;

static volatile adc_format_t s_format  = FMT_ASCII;
static volatile uint32_t     s_rate_hz = ADC_SAMPLE_RATE_DEFAULT_HZ;

static struct {
    volatile uint32_t samples;      /* отдано хосту (в очередь передачи)         */
    volatile uint32_t chunks;
    volatile uint32_t dropped;      /* чанки, не принятые транспортом (хост stalls) */
    volatile uint32_t overruns;     /* DMA перезаписал буфер, не дождавшись обработки */
} s_stats;

static uint16_t          s_buf[2][ADC_N];
static volatile bool     s_pending[2];
static uint8_t           s_scratch[TX_SCRATCH_BYTES];
static const uint16_t    s_dummy_tx = 0xFFFF;

static int s_dma_tx;
static int s_dma_rx[2];
static int s_dma_timer;

/* ---- Прерывание DMA ------------------------------------------------------ *
 * Когда буфер i заполнен, DMA по chain_to уже начал писать в буфер i^1.
 * Здесь только: переармировать i (адрес/счётчик, без запуска), пометить
 * готовность и разбудить job. Вся тяжёлая работа — в задаче.
 * ------------------------------------------------------------------------ */
static void dma_isr(void)
{
    uint32_t events = 0;

    for (int i = 0; i < 2; i++) {
        uint ch = (uint)s_dma_rx[i];
        if (dma_channel_get_irq1_status(ch)) {
            dma_channel_acknowledge_irq1(ch);
            dma_channel_set_write_addr(ch, s_buf[i], false);
            dma_channel_set_trans_count(ch, ADC_N, false);
            if (s_pending[i ^ 1]) {
                s_stats.overruns++;     /* соседний буфер ещё не обработан, а DMA в него уже пишет */
            }
            s_pending[i] = true;
            events |= EV_BUF(i);
        }
    }
    if (events) {
        iwp_job_notify_from_isr(&s_job, events);
    }
}

/* ---- Железо -------------------------------------------------------------- */

static void spi_drain(void)
{
    while (spi_is_busy(ADC_SPI_PORT)) {
        tight_loop_contents();
    }
    while (spi_is_readable(ADC_SPI_PORT)) {
        (void)spi_get_hw(ADC_SPI_PORT)->dr;
    }
}

static void hw_start(uint32_t rate_hz)
{
    spi_drain();
    s_pending[0] = s_pending[1] = false;

    /* Темп отсчётов задаёт DMA-таймер: rate = clk_sys * 1 / denom. */
    uint32_t denom = clock_get_hz(clk_sys) / rate_hz;
    if (denom < 1)      denom = 1;
    if (denom > 0xFFFF) denom = 0xFFFF;
    dma_timer_set_fraction((uint)s_dma_timer, 1, (uint16_t)denom);

    for (int i = 0; i < 2; i++) {
        dma_channel_config c = dma_channel_get_default_config((uint)s_dma_rx[i]);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
        channel_config_set_read_increment(&c, false);
        channel_config_set_write_increment(&c, true);
        channel_config_set_dreq(&c, spi_get_dreq(ADC_SPI_PORT, false));
        channel_config_set_chain_to(&c, (uint)s_dma_rx[i ^ 1]);
        dma_channel_configure((uint)s_dma_rx[i], &c, s_buf[i],
                              &spi_get_hw(ADC_SPI_PORT)->dr, ADC_N, false);
        dma_channel_acknowledge_irq1((uint)s_dma_rx[i]);
        dma_channel_set_irq1_enabled((uint)s_dma_rx[i], true);
    }

    /* TX гоняет холостые слова: ему важно только тактирование SPI. */
    dma_channel_config t = dma_channel_get_default_config((uint)s_dma_tx);
    channel_config_set_transfer_data_size(&t, DMA_SIZE_16);
    channel_config_set_read_increment(&t, false);
    channel_config_set_write_increment(&t, false);
    channel_config_set_dreq(&t, dma_get_timer_dreq((uint)s_dma_timer));
    dma_channel_configure((uint)s_dma_tx, &t, &spi_get_hw(ADC_SPI_PORT)->dr,
                          &s_dummy_tx, 0xFFFFFFFFu, false);

    dma_start_channel_mask((1u << s_dma_rx[0]) | (1u << s_dma_tx));
}

static void hw_stop(void)
{
    uint32_t mask = (1u << s_dma_tx) | (1u << s_dma_rx[0]) | (1u << s_dma_rx[1]);

    dma_channel_set_irq1_enabled((uint)s_dma_rx[0], false);
    dma_channel_set_irq1_enabled((uint)s_dma_rx[1], false);

    dma_hw->abort = mask;                   /* все три сразу: иначе chain_to перезапустит соседа */
    while (dma_hw->abort & mask) {
        tight_loop_contents();
    }
    dma_hw->ints1 = mask;                   /* сбросить возможные хвостовые флаги */
    spi_drain();
}

/* ---- Формирование пакета ------------------------------------------------- */

static size_t format_chunk(const uint16_t *src, uint32_t n, adc_format_t fmt, uint16_t seq)
{
    uint8_t *p = s_scratch;

    if (fmt == FMT_BINARY) {
        p[0] = PACKET_MAGIC_0;
        p[1] = PACKET_MAGIC_1;
        p[2] = (uint8_t)(seq & 0xFF);
        p[3] = (uint8_t)(seq >> 8);
        p[4] = (uint8_t)(n & 0xFF);
        p[5] = (uint8_t)(n >> 8);
        for (uint32_t i = 0; i < n; i++) {
            p[BIN_HEADER_BYTES + 2 * i]     = (uint8_t)(src[i] & 0xFF);
            p[BIN_HEADER_BYTES + 2 * i + 1] = (uint8_t)(src[i] >> 8);
        }
        return BIN_HEADER_BYTES + 2 * (size_t)n;
    }

    for (uint32_t i = 0; i < n; i++) {
        memcpy(p, ADC_ASCII_PREFIX, sizeof(ADC_ASCII_PREFIX) - 1);
        p += sizeof(ADC_ASCII_PREFIX) - 1;

        uint16_t v = src[i];
        char digits[5];
        int d = 0;
        do {
            digits[d++] = (char)('0' + (v % 10));
            v /= 10;
        } while (v);
        while (d) {
            *p++ = (uint8_t)digits[--d];
        }
        *p++ = '\r';
        *p++ = '\n';
    }
    return (size_t)(p - s_scratch);
}

/* ---- Задача -------------------------------------------------------------- */

static void adc_job(iwp_job_t *job, void *arg)
{
    uint32_t     remaining = (uint32_t)(uintptr_t)arg;     /* 0 = без ограничения */
    const bool   limited   = (remaining != 0);
    adc_format_t fmt       = s_format;
    uint32_t     rate      = s_rate_hz;
    uint16_t     seq       = 0;
    int          next      = 0;

    memset((void *)&s_stats, 0, sizeof(s_stats));
    hw_start(rate);

    while (!iwp_job_stop_requested(job)) {
        iwp_job_wait(job, pdMS_TO_TICKS(250));

        while (s_pending[next] && !iwp_job_stop_requested(job)) {
            uint32_t n = ADC_N;
            if (limited && n > remaining) {
                n = remaining;
            }

            size_t len = format_chunk(s_buf[next], n, fmt, seq++);
            s_pending[next] = false;                /* буфер свободен — данные уже скопированы */
            next ^= 1;

            size_t sent = iwp_transport_write(s_scratch, len, pdMS_TO_TICKS(SEND_TIMEOUT_MS));
            if (sent == len) {
                s_stats.chunks++;
                s_stats.samples += n;
            } else {
                s_stats.dropped++;
            }

            if (limited) {
                remaining -= n;
                if (remaining == 0) {
                    goto done;
                }
            }
        }
    }
done:
    hw_stop();
}

/* ---- Инициализация ------------------------------------------------------- */

bool adc_dump_init(void)
{
    spi_init(ADC_SPI_PORT, ADC_SPI_BAUD);
    spi_set_format(ADC_SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(ADC_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(ADC_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(ADC_PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(ADC_PIN_CS,   GPIO_FUNC_SPI);

    s_dma_tx    = dma_claim_unused_channel(true);
    s_dma_rx[0] = dma_claim_unused_channel(true);
    s_dma_rx[1] = dma_claim_unused_channel(true);
    s_dma_timer = dma_claim_unused_timer(true);

    /* Общий обработчик: не конфликтует с другим кодом на DMA_IRQ_1. */
    irq_add_shared_handler(DMA_IRQ_1, dma_isr, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(DMA_IRQ_1, true);

    return iwp_job_init(&s_job, "adc_dump", adc_job, ADC_JOB_STACK, PRIO_JOB);
}

/* ---- SCPI ---------------------------------------------------------------- */

static uint32_t rate_min(void) { return clock_get_hz(clk_sys) / 0xFFFFu + 1; }
static uint32_t rate_max(void) { return ADC_SPI_BAUD / 20u; }   /* 16 тактов + зазор CS */

scpi_result_t adc_dump_scpi_start(scpi_t *context)
{
    scpi_parameter_t p;
    uint32_t count = 0;

    if (SCPI_Parameter(context, &p, FALSE)) {
        if (!SCPI_ParamToUInt32(context, &p, &count)) {
            return SCPI_RES_ERR;
        }
    }
    if (!iwp_job_start(&s_job, (void *)(uintptr_t)count)) {
        SCPI_ErrorPush(context, SCPI_ERROR_SETTINGS_CONFLICT);   /* уже запущен */
        return SCPI_RES_ERR;
    }
    return SCPI_RES_OK;
}

scpi_result_t adc_dump_scpi_stop(scpi_t *context)
{
    if (!iwp_job_stop(&s_job, pdMS_TO_TICKS(STOP_TIMEOUT_MS))) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }
    return SCPI_RES_OK;
}

scpi_result_t adc_dump_scpi_state_q(scpi_t *context)
{
    SCPI_ResultMnemonic(context, iwp_job_is_running(&s_job) ? "RUN" : "IDLE");
    return SCPI_RES_OK;
}

scpi_result_t adc_dump_scpi_format(scpi_t *context)
{
    int32_t v;
    if (!SCPI_ParamChoice(context, s_format_choices, &v, TRUE)) {
        return SCPI_RES_ERR;
    }
    if (iwp_job_is_running(&s_job)) {
        SCPI_ErrorPush(context, SCPI_ERROR_SETTINGS_CONFLICT);
        return SCPI_RES_ERR;
    }
    s_format = (adc_format_t)v;
    return SCPI_RES_OK;
}

scpi_result_t adc_dump_scpi_format_q(scpi_t *context)
{
    const char *name = NULL;
    SCPI_ChoiceToName(s_format_choices, (int32_t)s_format, &name);
    SCPI_ResultMnemonic(context, name ? name : "?");
    return SCPI_RES_OK;
}

scpi_result_t adc_dump_scpi_rate(scpi_t *context)
{
    uint32_t hz;
    if (!SCPI_ParamUInt32(context, &hz, TRUE)) {
        return SCPI_RES_ERR;
    }
    if (hz < rate_min() || hz > rate_max()) {
        SCPI_ErrorPush(context, SCPI_ERROR_DATA_OUT_OF_RANGE);
        return SCPI_RES_ERR;
    }
    if (iwp_job_is_running(&s_job)) {
        SCPI_ErrorPush(context, SCPI_ERROR_SETTINGS_CONFLICT);
        return SCPI_RES_ERR;
    }
    s_rate_hz = hz;
    return SCPI_RES_OK;
}

scpi_result_t adc_dump_scpi_rate_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, s_rate_hz);
    return SCPI_RES_OK;
}

scpi_result_t adc_dump_scpi_stats_q(scpi_t *context)
{
    SCPI_ResultUInt32(context, s_stats.samples);
    SCPI_ResultUInt32(context, s_stats.chunks);
    SCPI_ResultUInt32(context, s_stats.dropped);
    SCPI_ResultUInt32(context, s_stats.overruns);
    return SCPI_RES_OK;
}
