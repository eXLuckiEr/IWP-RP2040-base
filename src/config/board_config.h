#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/* ===========================================================================
 *  Единственное место, где живут пины, приоритеты, стеки и размеры буферов.
 *  Ядро (src/core) читает только секции RTOS / TRANSPORT / SCPI.
 *  Секции APP_* относятся к примерам из src/app и src/modules.
 * ======================================================================== */

/* ---- RTOS: приоритеты (чем больше число, тем выше) ----------------------- */
#define PRIO_TRANSPORT      3   /* USB <-> потоковые буферы: должен быть отзывчивым */
#define PRIO_SCPI           2   /* разбор команд                                    */
#define PRIO_JOB            1   /* фоновые задачи по команде (дамп, измерения...)   */
#define PRIO_HEARTBEAT      1

/* ---- RTOS: стеки (в словах, 1 слово = 4 байта) --------------------------- */
#define STACK_TRANSPORT     512
#define STACK_SCPI          1024
#define STACK_JOB_DEFAULT   768
#define STACK_HEARTBEAT     configMINIMAL_STACK_SIZE

/* ---- TRANSPORT ----------------------------------------------------------- */
#define TRANSPORT_RX_BUFFER_BYTES   256     /* USB -> SCPI                        */
#define TRANSPORT_TX_BUFFER_BYTES   8192    /* всё, что уходит хосту              */
#define TRANSPORT_TX_CHUNK_BYTES    256     /* максимум за один вызов в USB       */
#define TRANSPORT_BREAK_BYTE        0x03    /* Ctrl-C из потока = ABORt всех job  */

/* ---- SCPI ---------------------------------------------------------------- */
#define SCPI_INPUT_BUFFER_LENGTH    256     /* одна команда не длиннее            */
#define SCPI_ERROR_QUEUE_SIZE       17
#define SCPI_RESPONSE_BUFFER_BYTES  512     /* ответ копится и уходит целиком     */
#define SCPI_RESPONSE_TIMEOUT_MS    200
#define SCPI_ECHO_ERRORS            0       /* 1 = печатать "**ERROR" в поток     */
#define SCPI_IDN_MANUFACTURER       "MY COMPANY"
#define SCPI_IDN_MODEL              "RP2040 SCPI BASE"
#define SCPI_IDN_SERIAL             NULL
#define SCPI_IDN_FIRMWARE           "0.1"

/* ---- SYSINFO: встроенный датчик температуры RP2040 ----------------------- */
#define SYSINFO_ADC_VREF_VOLTS      3.3f    /* опорное напряжение АЦП (на Pico — 3V3 через фильтр) */
#define SYSINFO_TEMP_OFFSET_C       0.0f    /* ваша калибровочная поправка, °C                    */
#define SYSINFO_TEMP_AVERAGE        16      /* число выборок для усреднения                       */

/* ---- Какие примеры включены (дублируются из CMake, см. CMakeLists.txt) --- */
#ifndef APP_ADC_DUMP
#define APP_ADC_DUMP        0
#endif
#ifndef APP_I2C_TOOLS
#define APP_I2C_TOOLS       0
#endif
#ifndef APP_DISPLAY
#define APP_DISPLAY         0
#endif

/* ---- APP_ADC_DUMP: SPI АЦП (например ADS7886) через DMA ------------------ */
#define ADC_SPI_PORT        spi0
#define ADC_PIN_MISO        16
#define ADC_PIN_CS          17
#define ADC_PIN_SCK         18
#define ADC_PIN_MOSI        19
#define ADC_SPI_BAUD        1000000u
#define ADC_SAMPLE_RATE_DEFAULT_HZ  20000u
#define ADC_DMA_BUF_SAMPLES 512             /* два буфера (ping-pong)             */
#define ADC_ASCII_PREFIX    ">Val:"         /* строка дампа: ">Val:1234\r\n"      */
#define ADC_JOB_STACK       STACK_JOB_DEFAULT

/* ---- APP_I2C_TOOLS ------------------------------------------------------- */
#define I2C_TOOLS_PORT      i2c1
#define I2C_TOOLS_SDA       26
#define I2C_TOOLS_SCL       27
#define I2C_TOOLS_BAUD      400000u

#endif /* BOARD_CONFIG_H */
