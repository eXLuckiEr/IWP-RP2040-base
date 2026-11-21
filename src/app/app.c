/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <stdbool.h>
#include "app.h"
#include "config/board_config.h"

#if APP_ADC_DUMP
#include "app/adc_dump.h"
#endif
#if APP_I2C_TOOLS
#include "modules/i2c_tools/scpi_i2c.h"
#endif
#if APP_DISPLAY
#include "modules/display/scpi_display.h"
#endif

bool app_init(void)
{
    bool ok = true;
#if APP_ADC_DUMP
    ok &= adc_dump_init();
#endif
#if APP_I2C_TOOLS
    ok &= i2c_tools_init();
#endif
#if APP_DISPLAY
    ok &= display_init();
#endif
    return ok;
}
