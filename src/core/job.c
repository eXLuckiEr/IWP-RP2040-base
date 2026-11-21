#include "job.h"
#include <string.h>

/* Служебные биты уведомления (верхние два). Остальные 30 — пользовательские. */
#define JOB_NOTIFY_START    (1u << 31)
#define JOB_NOTIFY_STOP     (1u << 30)
#define JOB_EV_IDLE         (1u << 0)

static job_t *s_jobs[JOB_MAX_JOBS];
static uint8_t s_job_count;

static void job_task(void *param)
{
    job_t *job = (job_t *)param;
    uint32_t bits;

    for (;;) {
        /* Парковка: ждём START. Все накопившиеся биты сбрасываем на выходе. */
        bits = 0;
        xTaskNotifyWait(0, 0xFFFFFFFFu, &bits, portMAX_DELAY);
        if (!(bits & JOB_NOTIFY_START)) {
            continue;   /* лишний STOP/пользовательский бит в простое */
        }

        job->fn(job, job->arg);

        job->running = false;
        xEventGroupSetBits(job->idle_event, JOB_EV_IDLE);
    }
}

bool job_init(job_t *job, const char *name, job_fn_t fn,
              uint16_t stack_words, UBaseType_t priority)
{
    if (job == NULL || fn == NULL || s_job_count >= JOB_MAX_JOBS) {
        return false;
    }
    memset(job, 0, sizeof(*job));
    job->name = name;
    job->fn = fn;
    job->idle_event = xEventGroupCreate();
    if (job->idle_event == NULL) {
        return false;
    }
    xEventGroupSetBits(job->idle_event, JOB_EV_IDLE);

    if (xTaskCreate(job_task, name, stack_words, job, priority, &job->task) != pdPASS) {
        vEventGroupDelete(job->idle_event);
        return false;
    }
    s_jobs[s_job_count++] = job;
    return true;
}

bool job_start(job_t *job, void *arg)
{
    bool ok = false;

    taskENTER_CRITICAL();
    if (!job->running) {
        job->running = true;
        job->stop_requested = false;
        job->arg = arg;
        ok = true;
    }
    taskEXIT_CRITICAL();

    if (ok) {
        xEventGroupClearBits(job->idle_event, JOB_EV_IDLE);
        xTaskNotify(job->task, JOB_NOTIFY_START, eSetBits);
    }
    return ok;
}

void job_request_stop(job_t *job)
{
    if (job->running) {
        job->stop_requested = true;
        xTaskNotify(job->task, JOB_NOTIFY_STOP, eSetBits);
    }
}

bool job_stop(job_t *job, TickType_t timeout)
{
    if (!job->running) {
        return true;
    }
    job_request_stop(job);
    EventBits_t ev = xEventGroupWaitBits(job->idle_event, JOB_EV_IDLE,
                                         pdFALSE, pdFALSE, timeout);
    return (ev & JOB_EV_IDLE) != 0;
}

bool job_is_running(const job_t *job)
{
    return job->running;
}

void job_request_stop_all(void)
{
    for (uint8_t i = 0; i < s_job_count; i++) {
        job_request_stop(s_jobs[i]);
    }
}

bool job_stop_all(TickType_t timeout)
{
    TickType_t start = xTaskGetTickCount();
    bool all = true;

    job_request_stop_all();
    for (uint8_t i = 0; i < s_job_count; i++) {
        TickType_t used = xTaskGetTickCount() - start;
        TickType_t left = (used >= timeout) ? 0 : (timeout - used);
        if (!job_stop(s_jobs[i], left)) {
            all = false;
        }
    }
    return all;
}

uint32_t job_wait(job_t *job, TickType_t timeout)
{
    (void)job;
    uint32_t bits = 0;
    if (xTaskNotifyWait(0, 0xFFFFFFFFu, &bits, timeout) != pdTRUE) {
        return 0;
    }
    return bits & JOB_USER_BITS_MASK;
}

void job_notify(job_t *job, uint32_t bits)
{
    xTaskNotify(job->task, bits & JOB_USER_BITS_MASK, eSetBits);
}

void job_notify_from_isr(job_t *job, uint32_t bits)
{
    BaseType_t woken = pdFALSE;
    xTaskNotifyFromISR(job->task, bits & JOB_USER_BITS_MASK, eSetBits, &woken);
    portYIELD_FROM_ISR(woken);
}
