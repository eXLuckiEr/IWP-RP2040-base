#include "scpi_display.h"

#include "driver_ssd1309_advance.h"

bool display_init(void)
{
    return ssd1309_advance_init(SSD1309_INTERFACE_SPI, SSD1309_ADDR_SA0_0) == 0;
}

static scpi_result_t from_status(scpi_t *context, uint8_t status)
{
    if (status != 0) {
        SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        return SCPI_RES_ERR;
    }
    return SCPI_RES_OK;
}

scpi_result_t scpi_display_state(scpi_t *context)
{
    scpi_bool_t on;
    if (!SCPI_ParamBool(context, &on, TRUE)) {
        return SCPI_RES_ERR;
    }
    return from_status(context, on ? ssd1309_advance_display_on()
                                   : ssd1309_advance_display_off());
}

scpi_result_t scpi_display_clear(scpi_t *context)
{
    return from_status(context, ssd1309_advance_clear());
}

scpi_result_t scpi_display_write(scpi_t *context)
{
    char text[128];
    size_t len = 0;

    if (!SCPI_ParamCopyText(context, text, sizeof(text), &len, TRUE)) {
        return SCPI_RES_ERR;
    }
    if (ssd1309_advance_clear() != 0) {
        return from_status(context, 1);
    }
    return from_status(context, ssd1309_advance_string(1, 1, text, (uint16_t)len, 1, SSD1309_FONT_16));
}
