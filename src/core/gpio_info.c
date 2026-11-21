/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "gpio_info.h"

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/structs/iobank0.h"
#include "hardware/structs/padsbank0.h"

/* Номера FUNCSEL (даташит RP2040, «GPIO Functions»). */
enum { F_XIP = 0, F_SPI = 1, F_UART = 2, F_I2C = 3, F_PWM = 4, F_SIO = 5,
       F_PIO0 = 6, F_PIO1 = 7, F_CLOCK = 8, F_USB = 9, F_NULL = 31 };

void iwp_gpio_signal_name(unsigned pin, unsigned funcsel, bool input_enabled,
                          char *out, size_t out_size)
{
    static const char *const spi_sig[]  = { "RX", "CSN", "SCK", "TX" };
    static const char *const uart_sig[] = { "TX", "RX", "CTS", "RTS" };
    static const char *const usb_sig[]  = { "OVCUR_DET", "VBUS_DET", "VBUS_EN" };

    switch (funcsel) {
    case F_SPI:
        snprintf(out, out_size, "SPI%u_%s", (pin / 8u) % 2u, spi_sig[pin % 4u]);
        break;
    case F_UART:
        snprintf(out, out_size, "UART%u_%s", (((pin >> 2) + 1u) >> 1) & 1u, uart_sig[pin % 4u]);
        break;
    case F_I2C:
        snprintf(out, out_size, "I2C%u_%s", (pin >> 1) & 1u, (pin & 1u) ? "SCL" : "SDA");
        break;
    case F_PWM:
        snprintf(out, out_size, "PWM%u_%c", (pin >> 1) & 7u, (pin & 1u) ? 'B' : 'A');
        break;
    case F_SIO:
        snprintf(out, out_size, "SIO");
        break;
    case F_PIO0:
        snprintf(out, out_size, "PIO0");
        break;
    case F_PIO1:
        snprintf(out, out_size, "PIO1");
        break;
    case F_CLOCK:
        if (pin >= 20 && pin <= 25) {
            static const char *const clk[] = { "GPIN0", "GPOUT0", "GPIN1", "GPOUT1", "GPOUT2", "GPOUT3" };
            snprintf(out, out_size, "CLOCK_%s", clk[pin - 20u]);
        } else {
            snprintf(out, out_size, "CLOCK");
        }
        break;
    case F_USB:
        snprintf(out, out_size, "USB_%s", usb_sig[pin % 3u]);
        break;
    case F_XIP:
        snprintf(out, out_size, "XIP");
        break;
    case F_NULL:
    default:
        /* ADC: нет FUNCSEL, аналоговый пин = функция NULL + выключенный цифровой вход. */
        if (pin >= 26 && pin <= 29 && !input_enabled) {
            snprintf(out, out_size, "ADC%u", pin - 26u);
        } else {
            snprintf(out, out_size, "NONE");
        }
        break;
    }
}

typedef struct {
    char        signal[20];
    bool        is_output;
    bool        level;
    const char *pull;
} pin_state_t;

static void read_pin(unsigned pin, pin_state_t *st)
{
    uint32_t status = io_bank0_hw->io[pin].status;
    bool ie = (padsbank0_hw->io[pin] & PADS_BANK0_GPIO0_IE_BITS) != 0;

    iwp_gpio_signal_name(pin, (unsigned)gpio_get_function(pin), ie, st->signal, sizeof(st->signal));

    st->is_output = (status & IO_BANK0_GPIO0_STATUS_OETOPAD_BITS) != 0;
    st->level     = (status & IO_BANK0_GPIO0_STATUS_INFROMPAD_BITS) != 0;

    bool up   = gpio_is_pulled_up(pin);
    bool down = gpio_is_pulled_down(pin);
    st->pull  = (up && down) ? "BOTH" : up ? "UP" : down ? "DOWN" : "NONE";
}

static bool param_pin(scpi_t *context, unsigned *pin)
{
    uint32_t v;
    if (!SCPI_ParamUInt32(context, &v, TRUE)) {
        return false;
    }
    if (v >= NUM_BANK0_GPIOS) {
        SCPI_ErrorPush(context, SCPI_ERROR_DATA_OUT_OF_RANGE);
        return false;
    }
    *pin = v;
    return true;
}

scpi_result_t scpi_gpio_state_q(scpi_t *context)
{
    unsigned pin;
    pin_state_t st;

    if (!param_pin(context, &pin)) {
        return SCPI_RES_ERR;
    }
    read_pin(pin, &st);
    SCPI_ResultText(context, st.signal);
    SCPI_ResultMnemonic(context, st.is_output ? "OUT" : "IN");
    SCPI_ResultInt32(context, st.level);
    SCPI_ResultMnemonic(context, st.pull);
    return SCPI_RES_OK;
}

scpi_result_t scpi_gpio_level_q(scpi_t *context)
{
    unsigned pin;
    pin_state_t st;

    if (!param_pin(context, &pin)) {
        return SCPI_RES_ERR;
    }
    read_pin(pin, &st);
    SCPI_ResultInt32(context, st.level);
    return SCPI_RES_OK;
}

scpi_result_t scpi_gpio_map_q(scpi_t *context)
{
    for (unsigned pin = 0; pin < NUM_BANK0_GPIOS; pin++) {
        pin_state_t st;
        char line[48];

        read_pin(pin, &st);
        snprintf(line, sizeof(line), "%u,%s,%s,%d,%s", pin, st.signal,
                 st.is_output ? "OUT" : "IN", st.level ? 1 : 0, st.pull);
        SCPI_ResultText(context, line);
    }
    return SCPI_RES_OK;
}
