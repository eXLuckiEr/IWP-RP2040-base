#include <FreeRTOS.h>
#include <task.h>

#include "pico/stdlib.h"

#include "config/board_config.h"
#include "core/transport.h"
#include "core/scpi_port.h"
#include "core/sysinfo.h"
#include "app/app.h"

/* Мигание светодиодом — признак того, что планировщик жив. */
static void heartbeat_task(void *param)
{
    (void)param;
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    for (;;) {
        gpio_xor_mask(1u << PICO_DEFAULT_LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void)
{
    stdio_init_all();

    /* Не вкладывайте вызовы внутрь configASSERT: в Release он может быть пустым. */
    bool ok = sysinfo_init();   /* до старта планировщика: читает UID из flash */
    ok = ok && transport_init();
    ok = ok && scpi_port_init();
    ok = ok && app_init();
    configASSERT(ok);
    (void)ok;
    xTaskCreate(heartbeat_task, "heartbeat", STACK_HEARTBEAT, NULL, PRIO_HEARTBEAT, NULL);

    vTaskStartScheduler();
    for (;;) {
        /* сюда управление не попадает */
    }
}
