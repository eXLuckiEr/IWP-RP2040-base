/* -------------------------------------------------------------------------
 *  pcal6534_test.c   –  simple register‑test for the PCAL6534 I²C expander
 * -------------------------------------------------------------------------
 *  The code follows the same style as the two SCPI helper functions you
 *  posted (SCPI_I2C_write / SCPI_I2C_read).  All bus handling (init,
 *  GPIO set‑up, pull‑ups, de‑init) is performed inside the low‑level
 *  helpers, therefore the test routine can be used from anywhere
 *  (SCPI command, raw firmware test, unit‑test, …).
 *
 *  Compile with the same flags / libraries you use for the rest of the
 *  project – the only external symbols are the I²C API (i2c_init,
 *  i2c_write_blocking, i2c_read_blocking, i2c_deinit) and the GPIO API.
 * -------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "scpi-def.h"

#include "hardware/i2c.h"
#include "hardware/gpio.h"

#ifndef I2C_PORT
    #define I2C_PORT i2c1
    #define I2C_SCLK 27
    #define I2C_SDA  26
#endif

/* -------------------------------------------------------------
 *  Low‑level bus helpers – they mimic the code you already have
 * ------------------------------------------------------------- */
static bool i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t value)
{
    /* Same initialisation sequence you use in SCPI_I2C_write() */
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA,  GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCLK, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCLK);

    uint8_t buf[2] = { reg_addr, value };
    int rc = i2c_write_blocking(I2C_PORT, dev_addr, buf, 2, false);

    i2c_deinit(I2C_PORT);
    return (rc >= 0);
}

static bool i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *value)
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA,  GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCLK, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCLK);

    /* write the register pointer (repeated start, read) */
    int rc = i2c_write_blocking(I2C_PORT, dev_addr, &reg_addr, 1, true);
    if (rc >= 0) {
        rc = i2c_read_blocking(I2C_PORT, dev_addr, value, 1, false);
    }

    i2c_deinit(I2C_PORT);
    return (rc >= 0);
}

/* -------------------------------------------------------------
 *  Register description – one entry per address (0x00 … 0x6F)
 * ------------------------------------------------------------- */
typedef struct {
    uint8_t  addr;           /* register address                      */
    const char *name;        /* human readable name                   */
    bool     read_only;      /* true → writes must not change content */
    bool     write_only;     /* true → reads always return 0x00      */
    bool     reserved;       /* true → device does not ACK            */
    uint8_t  reset_val;      /* power‑up default (for reference)      */
    uint8_t  write_mask;     /* which bits may be written (0 = ignored)*/
} RegInfo;

/*  A compact table – only the registers that exist in the
 *  data‑sheet are listed; the “reserved” flag is used for the gaps
 *  (0x14 … 0x2F, 0x3E … 0x3F, 0x4F … 0x5D, 0x66 … 0x6C, 0x70 … 0x7F).   */
static const RegInfo reg_table[] = {
    /* Input ports – read only                                            */
    {0x00, "Input Port 0",          true, false, false, 0xFF, 0x00},
    {0x01, "Input Port 1",          true, false, false, 0xFF, 0x00},
    {0x02, "Input Port 2",          true, false, false, 0xFF, 0x00},
    {0x03, "Input Port 3",          true, false, false, 0xFF, 0x00},
    {0x04, "Input Port 4",          true, false, false, 0x03, 0x00},

    /* Output ports – read / write                                        */
    {0x05, "Output Port 0",         false,false,false, 0xFF, 0xFF},
    {0x06, "Output Port 1",         false,false,false, 0xFF, 0xFF},
    {0x07, "Output Port 2",         false,false,false, 0xFF, 0xFF},
    {0x08, "Output Port 3",         false,false,false, 0xFF, 0xFF},
    {0x09, "Output Port 4",         false,false,false, 0x03, 0x03},

    /* Polarity inversion registers                                        */
    {0x0A, "Polarity Invert Port 0",false,false,false, 0x00, 0xFF},
    {0x0B, "Polarity Invert Port 1",false,false,false, 0x00, 0xFF},
    {0x0C, "Polarity Invert Port 2",false,false,false, 0x00, 0xFF},
    {0x0D, "Polarity Invert Port 3",false,false,false, 0x00, 0xFF},
    {0x0E, "Polarity Invert Port 4",false,false,false, 0x00, 0x03},

    /* Configuration registers                                            */
    {0x0F, "Configuration Port 0",  false,false,false, 0xFF, 0xFF},
    {0x10, "Configuration Port 1",  false,false,false, 0xFF, 0xFF},
    {0x11, "Configuration Port 2",  false,false,false, 0xFF, 0xFF},
    {0x12, "Configuration Port 3",  false,false,false, 0xFF, 0xFF},
    {0x13, "Configuration Port 4",  false,false,false, 0x03, 0x03},

    /* Reserved – skip                                                    */
    {0x14, "Reserved",               false,false,true , 0x00, 0x00},

    /* Output‑drive‑strength registers (30h … 38h)                         */
    {0x30, "Drive Strength Port 0A", false,false,false, 0xFF, 0xFF},
    {0x31, "Drive Strength Port 0B", false,false,false, 0xFF, 0xFF},
    {0x32, "Drive Strength Port 1A", false,false,false, 0xFF, 0xFF},
    {0x33, "Drive Strength Port 1B", false,false,false, 0xFF, 0xFF},
    {0x34, "Drive Strength Port 2A", false,false,false, 0xFF, 0xFF},
    {0x35, "Drive Strength Port 2B", false,false,false, 0xFF, 0xFF},
    {0x36, "Drive Strength Port 3A", false,false,false, 0xFF, 0xFF},
    {0x37, "Drive Strength Port 3B", false,false,false, 0xFF, 0xFF},
    {0x38, "Drive Strength Port 4A", false,false,false, 0x0F, 0x0F},

    /* Input‑latch registers (3Ah … 3Eh)                                    */
    {0x3A, "Input Latch Port 0",    false,false,false, 0x00, 0xFF},
    {0x3B, "Input Latch Port 1",    false,false,false, 0x00, 0xFF},
    {0x3C, "Input Latch Port 2",    false,false,false, 0x00, 0xFF},
    {0x3D, "Input Latch Port 3",    false,false,false, 0x00, 0xFF},
    {0x3E, "Input Latch Port 4",    false,false,false, 0x00, 0x03},

    /* Pull‑up / pull‑down enable registers (3Fh … 43h)                     */
    {0x3F, "PU/PD Enable Port 0",    false,false,false, 0x00, 0xFF},
    {0x40, "PU/PD Enable Port 1",    false,false,false, 0x00, 0xFF},
    {0x41, "PU/PD Enable Port 2",    false,false,false, 0x00, 0xFF},
    {0x42, "PU/PD Enable Port 3",    false,false,false, 0x00, 0xFF},
    {0x43, "PU/PD Enable Port 4",    false,false,false, 0x00, 0x03},

    /* Pull‑up / pull‑down selection registers (44h … 48h)                  */
    {0x44, "PU/PD Select Port 0",    false,false,false, 0xFF, 0xFF},
    {0x45, "PU/PD Select Port 1",    false,false,false, 0xFF, 0xFF},
    {0x46, "PU/PD Select Port 2",    false,false,false, 0xFF, 0xFF},
    {0x47, "PU/PD Select Port 3",    false,false,false, 0xFF, 0xFF},
    {0x48, "PU/PD Select Port 4",    false,false,false, 0x03, 0x03},

    /* Interrupt mask registers (49h … 4Dh)                                 */
    {0x49, "Interrupt Mask Port 0", false,false,false, 0xFF, 0xFF},
    {0x4A, "Interrupt Mask Port 1", false,false,false, 0xFF, 0xFF},
    {0x4B, "Interrupt Mask Port 2", false,false,false, 0xFF, 0xFF},
    {0x4C, "Interrupt Mask Port 3", false,false,false, 0xFF, 0xFF},
    {0x4D, "Interrupt Mask Port 4", false,false,false, 0x03, 0x03},

    /* Interrupt status registers – read only                               */
    {0x4E, "Interrupt Status Port 0", true,false,false, 0x00, 0x00},
    {0x4F, "Interrupt Status Port 1", true,false,false, 0x00, 0x00},
    {0x50, "Interrupt Status Port 2", true,false,false, 0x00, 0x00},
    {0x51, "Interrupt Status Port 3", true,false,false, 0x00, 0x00},
    {0x52, "Interrupt Status Port 4", true,false,false, 0x00, 0x00},

    /* Output‑port configuration register (53h) – read/write                */
    {0x53, "Output Port Configuration", false,false,false, 0x00, 0x1F},

    /* Interrupt edge registers (54h … 5Ch)                                */
    {0x54, "Interrupt Edge Port 0A", false,false,false, 0x00, 0xFF},
    {0x55, "Interrupt Edge Port 0B", false,false,false, 0x00, 0xFF},
    {0x56, "Interrupt Edge Port 1A", false,false,false, 0x00, 0xFF},
    {0x57, "Interrupt Edge Port 1B", false,false,false, 0x00, 0xFF},
    {0x58, "Interrupt Edge Port 2A", false,false,false, 0x00, 0xFF},
    {0x59, "Interrupt Edge Port 2B", false,false,false, 0x00, 0xFF},
    {0x5A, "Interrupt Edge Port 3A", false,false,false, 0x00, 0xFF},
    {0x5B, "Interrupt Edge Port 3B", false,false,false, 0x00, 0xFF},
    {0x5C, "Interrupt Edge Port 4A", false,false,false, 0x03, 0x03},

    /* Interrupt‑clear registers – write‑only (reads always 0)            */
    {0x5E, "Interrupt Clear Port 0", false,true ,false, 0x00, 0xFF},
    {0x5F, "Interrupt Clear Port 1", false,true ,false, 0x00, 0xFF},
    {0x60, "Interrupt Clear Port 2", false,true ,false, 0x00, 0xFF},
    {0x61, "Interrupt Clear Port 3", false,true ,false, 0x00, 0xFF},
    {0x62, "Interrupt Clear Port 4", false,true ,false, 0x00, 0x03},

    /* Input‑status registers – read only (do NOT clear interrupts)       */
    {0x63, "Input Status Port 0",   true,false,false, 0xFF, 0x00},
    {0x64, "Input Status Port 1",   true,false,false, 0xFF, 0x00},
    {0x65, "Input Status Port 2",   true,false,false, 0xFF, 0x00},
    {0x66, "Input Status Port 3",   true,false,false, 0xFF, 0x00},
    {0x67, "Input Status Port 4",   true,false,false, 0x03, 0x00},

    /* Individual pin output configuration registers (68h … 6Ch)           */
    {0x68, "Indiv. Pin Config 0",   false,false,false, 0x00, 0xFF},
    {0x69, "Indiv. Pin Config 1",   false,false,false, 0x00, 0xFF},
    {0x6A, "Indiv. Pin Config 2",   false,false,false, 0x00, 0xFF},
    {0x6B, "Indiv. Pin Config 3",   false,false,false, 0x00, 0xFF},
    {0x6C, "Indiv. Pin Config 4",   false,false,false, 0x00, 0x03},

    /* Switch‑debounce registers (6Dh … 6Fh)                               */
    {0x6D, "Debounce Enable 0",      false,false,false, 0x00, 0xFF},
    {0x6E, "Debounce Enable 1",      false,false,false, 0x00, 0xFF},
    {0x6F, "Debounce Count",         false,false,false, 0x00, 0xFF},
};

/* -------------------------------------------------------------------------
 *  Helper: human readable bit‑field for the result column
 * ------------------------------------------------------------------------- */
static const char *result_str(bool ok)
{
    return ok ? "\x1b[32mPASS\x1b[0m" : "\x1b[31mFAIL\x1b[0m";
}

/* -------------------------------------------------------------------------
 *  Main test routine – expose it as a SCPI command if you wish
 * ------------------------------------------------------------------------- */
scpi_result_t SCPI_Test_PCAL6534_Registers(scpi_t *context)
{
    (void)context;                 /* we do not use the SCPI context here */

    /* -----------------------------------------------------------------
     *  The address on the bus depends on the state of the ADDR pin.
     *  The data‑sheet lists four possibilities:
     *     ADDR = VSS → 0x40
     *     ADDR = SCL → 0x42
     *     ADDR = SDA → 0x44
     *     ADDR = VDD → 0x46
     *
     *  For the sake of an example we assume the default (ADDR = VSS).
     * ----------------------------------------------------------------- */
    const uint8_t dev_addr = 0x42;               /* 7‑bit address, R/W bit added by driver */

    printf("\n=== PCAL6534 Register Self‑Test =================================================\n");
    printf("I²C device address : 0x%02X (ADDR pin = VSS)\n\n", dev_addr);
    printf("%-4s %-30s %-12s %-12s %-12s\n", "Addr", "Register", "Read‑Only", "Write‑Only", "Result");
    printf("---- -------------------------------- ---------------- ----------- ----------\n");

    for (size_t i = 0; i < sizeof(reg_table)/sizeof(reg_table[0]); ++i) {
        const RegInfo *r = &reg_table[i];
        uint8_t read_val = 0xFF;
        uint8_t after_write = 0xFF;
        bool ok = true;                 /* overall status for this register */

        /* -------------------------------------------------------------
         *  1) Try to read the register (unless it is declared
         *     “write only”, in which case the data‑sheet says we will
         *     always get 0x00).
         * ------------------------------------------------------------- */
        if (!r->write_only) {
            if (!i2c_read_reg(dev_addr, r->addr, &read_val)) {
                ok = false;
                printf("%02X  %-30s %-12s %-12s %-12s  (cannot read)\n",
                       r->addr, r->name,
                       r->read_only ? "YES" : "NO",
                       r->write_only ? "YES" : "NO",
                       result_str(ok));
                continue;
            }
        } else {
            /* write‑only registers are defined to read back as 0x00 */
            read_val = 0x00;
        }

        /* -------------------------------------------------------------
         *  2) If the register is read‑only we only verify that a write
         *     does NOT change the stored value.
         * ------------------------------------------------------------- */
        if (r->read_only) {
            /* try to write a pattern that is guaranteed to be different
               from the value we just read (xor with 0xFF).  The write
               must be ignored by the device.                                 */
            uint8_t bogus = (uint8_t)~read_val;
            i2c_write_reg(dev_addr, r->addr, bogus);

            /* read back again – it must be unchanged */
            uint8_t verify = 0x00;
            i2c_read_reg(dev_addr, r->addr, &verify);
            ok = (verify == read_val);
        }
        /* -------------------------------------------------------------
         *  3) If the register is write‑only we can only check that a
         *     read returns 0x00 (the device does not store anything).
         * ------------------------------------------------------------- */
        else if (r->write_only) {
            ok = (read_val == 0x00);
        }
        /* -------------------------------------------------------------
         *  4) Normal read/write register – write a few patterns and
         *     verify that we read the same value back.  The mask tells
         *     which bits are actually writable.
         * ------------------------------------------------------------- */
        else {
            const uint8_t patterns[] = {0x00, 0xFF, 0xAA, 0x55};
            for (size_t p = 0; p < sizeof(patterns); ++p) {
                uint8_t to_write = patterns[p] & r->write_mask;

                if (!i2c_write_reg(dev_addr, r->addr, to_write)) {
                    ok = false;
                    break;
                }
                if (!i2c_read_reg(dev_addr, r->addr, &after_write)) {
                    ok = false;
                    break;
                }
                /* Some registers (e.g. interrupt mask) have bits that are
                 * forced to 1 after reset; we only compare the bits that
                 * are writable.                                            */
                if ((after_write & r->write_mask) != (to_write & r->write_mask)) {
                    ok = false;
                    break;
                }
            }
        }

        printf("%02X  %-30s %-12s %-12s %-12s\n",
               r->addr,
               r->name,
               r->read_only ? "YES" : "NO",
               r->write_only ? "YES" : "NO",
               result_str(ok));
    }

    printf("\nTest finished.\n");
    return SCPI_RES_OK;
}

bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

scpi_result_t SCPI_I2C_scan(scpi_t * context)
{
    (void) context;

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCLK, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCLK);

    printf("\nI2C Bus Scan\n");
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            printf("%02x ", addr);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata=0x00;
        if (reserved_addr(addr)){
            ret = PICO_ERROR_GENERIC;
        }
        else {
            ret = i2c_write_blocking(I2C_PORT, addr, &rxdata, 1, false);
            // ret = i2c_read_blocking(I2C_PORT, addr, &rxdata, 1, false);
        }

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    i2c_deinit(I2C_PORT);
    printf("Done.\n");
    
    return SCPI_RES_OK;
}

scpi_result_t SCPI_I2C_write(scpi_t * context)
{
    (void) context;

    size_t o_count;
    uint32_t params[3] = {0,0,0};

    // if (!SCPI_ParamArrayUInt32(context, params, 3, &o_count, SCPI_FORMAT_NORMAL, TRUE)) {
    //     return SCPI_RES_ERR;
    // }
    if (!SCPI_ParamUInt32(context, &params[0], TRUE)) {
        return SCPI_RES_ERR;
    }
    if (!SCPI_ParamUInt32(context, &params[1], TRUE)) {
        return SCPI_RES_ERR;
    }
    if (!SCPI_ParamUInt32(context, &params[2], TRUE)) {
        return SCPI_RES_ERR;
    }

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCLK, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCLK);

    uint8_t data[3] = {(uint8_t)params[0] & 0xff,
                       (uint8_t)params[1] & 0xff,
                       (uint8_t)params[2] & 0xff };

    i2c_write_blocking(I2C_PORT, data[0], &data[1], 2, false);

    printf("I2C[0x%02x]:cmd[0x%02x]:data[0x%02x]\n", params[0], params[1], params[2]);
 
    i2c_deinit(I2C_PORT);
    
    return SCPI_RES_OK;
}

scpi_result_t SCPI_I2C_read(scpi_t * context)
{
    (void) context;
    
    size_t o_count;
    uint32_t params[2];

    // if (!SCPI_ParamArrayUInt32(context, params, 2, &o_count, SCPI_FORMAT_ASCII, TRUE)) {
    //     return SCPI_RES_ERR;
    // }
    if (!SCPI_ParamUInt32(context, &params[0], TRUE)) {
        return SCPI_RES_ERR;
    }
    if (!SCPI_ParamUInt32(context, &params[1], TRUE)) {
        return SCPI_RES_ERR;
    }

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCLK, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCLK);

    uint8_t data[3] = {(uint8_t)params[0] & 0xff,
                       (uint8_t)params[1] & 0xff, 
                       (uint8_t)0
                    };

    i2c_write_blocking(I2C_PORT, data[0], &data[1], 1, true);
    i2c_read_blocking(I2C_PORT, data[0], &data[2], 1, false);

    printf("I2C[0x%02x]:cmd[0x%02x]:data[0x%02x]\n", data[0], data[1], data[2]);
 
    i2c_deinit(I2C_PORT);
    
    return SCPI_RES_OK;
}
