# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

# Name project
SET(ProjectName iwp-rp2040-base)
SET(SOURCE_FOLDER src)

# ---- Какие примеры/модули включить: cmake -DENABLE_I2C_TOOLS=ON .. ---------
option(ENABLE_ADC_DUMP  "Пример: DMA-дамп SPI-АЦП (SPI:ADC)"      ON)
option(ENABLE_I2C_TOOLS "Модуль: I2C:SCAN?/READ?/WRITE"            OFF)
option(ENABLE_DISPLAY   "Модуль: OLED SSD1309 (нужен libs/ssd1309)" OFF)

# Set any variables required for importing libraries
SET(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/FreeRTOS-Kernel)
SET(SCPI_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/scpi-parser/libscpi)
SET(SSD1309_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/ssd1309)

# Import those libraries
include(pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Define project
project(iwp-rp2040-base)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# ---- SCPI parser -------------------------------------------------------------
file(GLOB SCPI_SRCS ${SCPI_LIB_PATH}/src/*.c)
add_library(scpi_lib STATIC ${SCPI_SRCS})
# Полный список кодов ошибок SCPI (SCPI_ERROR_SETTINGS_CONFLICT и др.). Макрос должен быть
# одинаковым в библиотеке и в прошивке, поэтому PUBLIC.
target_compile_definitions(scpi_lib PUBLIC USE_FULL_ERROR_LIST=1)
target_include_directories(scpi_lib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${SCPI_LIB_PATH}/inc
)

# ---- Ядро: транспорт + SCPI + фоновые задачи ---------------------------------
add_executable(iwp-rp2040-base
    ${SOURCE_FOLDER}/main.c
    ${SOURCE_FOLDER}/core/job.c
    ${SOURCE_FOLDER}/core/transport.c
    ${SOURCE_FOLDER}/core/scpi_port.c
    ${SOURCE_FOLDER}/core/scpi_commands.c
    ${SOURCE_FOLDER}/core/sysinfo.c
    ${SOURCE_FOLDER}/core/gpio_info.c
    ${SOURCE_FOLDER}/app/app.c
)

target_include_directories(iwp-rp2040-base PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${SOURCE_FOLDER}
)

set(APP_LIBS "")

# ---- Примеры / модули --------------------------------------------------------
if(ENABLE_ADC_DUMP)
    target_sources(iwp-rp2040-base PRIVATE ${SOURCE_FOLDER}/app/adc_dump.c)
    target_compile_definitions(iwp-rp2040-base PRIVATE APP_ADC_DUMP=1)
    list(APPEND APP_LIBS hardware_dma hardware_spi hardware_clocks hardware_irq)
endif()

if(ENABLE_I2C_TOOLS)
    target_sources(iwp-rp2040-base PRIVATE ${SOURCE_FOLDER}/modules/i2c_tools/scpi_i2c.c)
    target_compile_definitions(iwp-rp2040-base PRIVATE APP_I2C_TOOLS=1)
    list(APPEND APP_LIBS hardware_i2c)
endif()

if(ENABLE_DISPLAY)
    file(GLOB SSD1309_SRCS ${SSD1309_LIB_PATH}/src/*.c)
    add_library(ssd1309_lib STATIC ${SSD1309_SRCS})
    target_include_directories(ssd1309_lib PRIVATE ${CMAKE_CURRENT_LIST_DIR} ${SSD1309_LIB_PATH}/src)

    target_sources(iwp-rp2040-base PRIVATE
        ${SOURCE_FOLDER}/modules/display/scpi_display.c
        ${SOURCE_FOLDER}/modules/display/driver/driver_ssd1309_interface.c
        ${SOURCE_FOLDER}/modules/display/driver/driver_ssd1309_advance.c
    )
    target_include_directories(iwp-rp2040-base PRIVATE
        ${SOURCE_FOLDER}/modules/display/driver
        ${SSD1309_LIB_PATH}/src
    )
    target_compile_definitions(iwp-rp2040-base PRIVATE APP_DISPLAY=1)
    list(APPEND APP_LIBS ssd1309_lib hardware_spi)
endif()

# USB-CDC — канал SCPI (см. core/transport.c). UART-stdio выключен.
pico_enable_stdio_uart(iwp-rp2040-base 0)
pico_enable_stdio_usb(iwp-rp2040-base 1)

target_link_libraries(iwp-rp2040-base
    pico_stdlib
    pico_unique_id
    hardware_adc
    hardware_clocks
    scpi_lib
    FreeRTOS-Kernel-Heap4
    ${APP_LIBS}
)

pico_add_extra_outputs(iwp-rp2040-base)
