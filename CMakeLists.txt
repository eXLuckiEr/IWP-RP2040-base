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
SET(ProjectName rp2040-spi)
SET(SOURCE_FOLDER src)

# Set any variables required for importing libraries
SET(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/FreeRTOS-Kernel)
SET(SCPI_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/scpi-parser/libscpi)
SET(SSD1309_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libs/ssd1309)

# Import those libraries
include(pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Define project
project(rp2040-spi)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Добавьте исходные файлы SCPI в проект
file(GLOB SCPI_SRCS ${SCPI_LIB_PATH}/src/*.c)
add_library(scpi_lib STATIC ${SCPI_SRCS})

target_include_directories(scpi_lib PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${SCPI_LIB_PATH}/inc
)

# Добавьте исходные файлы SSD1309 в проект
file(GLOB SSD1309_SRCS ${SSD1309_LIB_PATH}/src/*.c)
add_library(ssd1309_lib STATIC ${SSD1309_SRCS})

target_include_directories(ssd1309_lib PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${SSD1309_LIB_PATH}/src
)

add_executable(rp2040-spi
    ${SOURCE_FOLDER}/main.c
    ${SOURCE_FOLDER}/scpi-extensions/scpi-def.c
    ${SOURCE_FOLDER}/scpi-extensions/SCPI_Test_PCAL6534.c
    ${SOURCE_FOLDER}/scpi-extensions/SCPI_Test_SSD1309.c
    ${SOURCE_FOLDER}/scpi-extensions/SCPI_Test_ADS7886.c
    ${SOURCE_FOLDER}/ssd1309-driver/driver_ssd1309_interface.c
    ${SOURCE_FOLDER}/ssd1309-driver/driver_ssd1309_advance.c
    ${SOURCE_FOLDER}/hardware/uart_handle.c
    ${SOURCE_FOLDER}/hardware/dma_adc.c
)

target_include_directories(rp2040-spi PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${SCPI_LIB_PATH}/inc
    ${SSD1309_LIB_PATH}/src
    ${SOURCE_FOLDER}/
    ${SOURCE_FOLDER}/scpi-extensions/
    ${SOURCE_FOLDER}/ssd1309-driver/
)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(rp2040-spi 0)
pico_enable_stdio_usb(rp2040-spi 1)

target_link_libraries(rp2040-spi
    pico_stdlib
    hardware_dma
    hardware_spi
    hardware_i2c
    scpi_lib 
    ssd1309_lib
    FreeRTOS-Kernel-Heap4 
    )

pico_add_extra_outputs(rp2040-spi)

# if you have anything in "lib" folder then uncomment below - remember to add a CMakeLists.txt
# file to the "lib" directory
#add_subdirectory(lib)