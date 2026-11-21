# RP2040 SCPI Base

Стартовая точка для устройств на RP2040, которыми нужно управлять снаружи:
**FreeRTOS + SCPI поверх USB-CDC + фоновые задачи, которые не блокируют приём команд**.

## Архитектура

```
 хост ──USB-CDC──► transport ──RX stream──► scpi (разбор) ──► SCPI-колбэк
   ▲                  ▲                                          │ job_start()/job_stop()
   │                  └──────── TX stream ◄── transport_write() ◄┤
   └──────────── SCPI-ответы и потоки данных ◄──────────────── job (фоновая задача)
```

```
src/
├── main.c                 создаёт задачи, больше ничего
├── config/board_config.h  пины, приоритеты, стеки, размеры буферов, *IDN?
├── core/                  БАЗА — трогать редко
│   ├── transport.{c,h}    USB-CDC <-> потоки байт, потокобезопасный transport_write()
│   ├── scpi_port.{c,h}    libscpi + FreeRTOS, буферизация ответов
│   ├── scpi_commands.c    таблица команд (IEEE 488.2, SYST:*, ABORt) + подключение модулей
│   └── job.{c,h}          фоновая задача со start/stop
├── app/                   ВАШ КОД
│   ├── app.c              инициализация примеров/модулей
│   └── adc_dump.{c,h}     пример: DMA-дамп SPI-АЦП, ASCII/бинарный, START/STOP
└── modules/               опциональные модули (выключены по умолчанию)
    ├── i2c_tools/         I2C:SCAN? / READ? / WRITE
    └── display/           OLED SSD1309
```

Примеры включаются в CMake: `-DENABLE_ADC_DUMP=ON|OFF`, `-DENABLE_I2C_TOOLS=ON`, `-DENABLE_DISPLAY=ON`.

## Как не блокировать SCPI

SCPI-колбэк выполняется в SCPI-задаче: пока он работает, следующие команды не разбираются.
Поэтому правило одно: **колбэк быстро возвращается, долгая работа живёт в `job`**.

```c
static job_t s_job;

static void my_job(job_t *job, void *arg) {
    while (!job_stop_requested(job)) {
        measure_and_send();                    // transport_write(...)
        job_wait(job, pdMS_TO_TICKS(100));     // просыпается раньше по STOP
    }
    cleanup();
}

job_init(&s_job, "my", my_job, STACK_JOB_DEFAULT, PRIO_JOB);   // один раз, в *_init()

scpi_result_t my_start(scpi_t *c) { return job_start(&s_job, NULL) ? SCPI_RES_OK : err(c); }
scpi_result_t my_stop(scpi_t *c)  { job_stop(&s_job, pdMS_TO_TICKS(500)); return SCPI_RES_OK; }
```

Остановить дамп можно четырьмя способами: `ADC:DUMP:STOP`, `ABORt`, `*RST`, а также
байтом `0x03` (Ctrl-C) прямо в потоке — он перехватывается в `transport` в обход
парсера, и ещё автоматически при отключении USB.

### Как отделить конец потока от ответов

`STOP` возвращается только после остановки потока, а ответы SCPI уходят целыми
сообщениями (не вклиниваются внутрь чанка данных). Поэтому:

```
ADC:DUMP:STOP;*OPC?     → всё до "1" — данные дампа, после — тишина
```

## Пример SCPI-команд (adc_dump)

| Команда | Действие |
|---|---|
| `ADC:DUMP:STARt [N]` | запуск; `N` отсчётов или без `N` — до `STOP` |
| `ADC:DUMP:STOP` | остановка |
| `ADC:DUMP:STATe?` | `IDLE` / `RUN` |
| `ADC:DUMP:FORMat ASCii\|BINary` | формат (только в `IDLE`) |
| `ADC:DUMP:RATE <Гц>` | частота отсчётов (только в `IDLE`) |
| `ADC:DUMP:STATistics?` | `отсчётов,чанков,потеряно_чанков,переполнений_DMA` |
| `ABORt` | остановить все фоновые задачи |
| `SYSTem:ERRor?` | очередь ошибок (ошибки в поток не печатаются, см. `SCPI_ECHO_ERRORS`) |

Форматы:
- ASCII: `>Val:1234\r\n` на отсчёт (префикс — `ADC_ASCII_PREFIX`);
- BINary: пакеты `A5 5A | seq u16 | n u16 | n × u16` (little-endian). `seq` растёт на 1 за пакет,
  по разрыву хост видит потерю данных.

Если хост не успевает читать, чанки отбрасываются (счётчик `dropped`), а не копятся и не вешают устройство.

## Сборка

```bash
git submodule update --init --recursive
export PICO_SDK_PATH=/path/to/pico-sdk   # или Pico VS Code extension
mkdir build && cd build
cmake .. -DENABLE_ADC_DUMP=ON
make -j
```

Библиотеки: FreeRTOS-Kernel (MIT), scpi-parser (BSD-2), ssd1309 libdriver (MIT, только для модуля display).

## Что убрано относительно прежней версии

Демо-команды дисплея (`PICTURE/SQUARE/POINT/SLOW`), регистровый тест PCAL6534,
заглушки `MEASure:*`, неиспользуемый `TEST_Chanlst`, `printf` в SCPI-колбэках
(они уходили в тот же USB-поток, что и данные), «UART»-названия для USB-CDC.
Всё это остаётся в истории git ветки `main`.
