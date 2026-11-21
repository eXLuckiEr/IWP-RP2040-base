# Сторонние компоненты и их лицензии

Собственный код проекта лицензирован под MPL-2.0 (см. `LICENSE`); ниже — только чужие компоненты.

Этот файл нужно поставлять вместе с исходниками и вместе с прошивкой (.uf2/.elf)
в документации устройства или в окне «О программе» IWP.

| Компонент | Где | Лицензия | Правообладатель |
|---|---|---|---|
| FreeRTOS-Kernel | `libs/FreeRTOS-Kernel` (сабмодуль), `FreeRTOSConfig.h` | MIT | Amazon.com, Inc. |
| scpi-parser (libscpi) | `libs/scpi-parser` (сабмодуль), производные: `src/core/scpi_commands.c`, `scpi_port.c`, `scpi_port.h` | BSD 2-Clause | Jan Breuer, 2012–2018 |
| LibDriver SSD1309 | `libs/ssd1309` (сабмодуль), копии в `src/modules/display/driver/` | MIT | LibDriver, 2015–present |
| Raspberry Pi Pico SDK | вне репозитория, компонуется в прошивку; `pico_sdk_import.cmake` | BSD 3-Clause | Raspberry Pi (Trading) Ltd. |
| TinyUSB (входит в Pico SDK) | вне репозитория, компонуется в прошивку (USB-CDC) | MIT | Ha Thach |

Полные тексты: `libs/*/LICENSE`, а также `LICENSE.TXT` в Pico SDK и `LICENSE` в TinyUSB
(`<pico-sdk>/lib/tinyusb`). При публикации прошивки приложите их тексты.
