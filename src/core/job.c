/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "job.h"
#include <string.h>

/* Служебные биты уведомления (верхние два). Остальные 30 — пользовательские. */
#define IWP_JOB_NOTIFY_START    (1u << 31)
#define IWP_JOB_NOTIFY_STOP     (1u << 30)
#define IWP_JOB_EV_IDLE         (1u << 0)

static iwp_job_t *s_jobs[IWP_JOB_MAX_JOBS];
static uint8_t s_job_count;

static void iwp_job_task(void *param)
{
    iwp_job_t *job = (iwp_job_t *)param;
    uint32_t bits;

    for (;;) {
        /* Парковка: ждём START. Все накопившиеся биты сбрасываем на выходе. */
        bits = 0;
        xTaskNotifyWait(0, 0xFFFFFFFFu, &bits, portMAX_DELAY);
        if (!(bits & IWP_JOB_NOTIFY_START)) {
            continue;   /* лишний STOP/пользовательский бит в простое */
        }

        job->fn(job, job->arg);

        job->running = false;
        xEventGroupSetBits(job->idle_event, IWP_JOB_EV_IDLE);
    }
}

bool iwp_job_init(iwp_job_t *job, const char *name, iwp_job_fn_t fn,
              uint16_t stack_words, UBaseType_t priority)
{
    if (job == NULL || fn == NULL || s_job_count >= IWP_JOB_MAX_JOBS) {
        return false;
    }
    memset(job, 0, sizeof(*job));
    job->name = name;
    job->fn = fn;
    job->idle_event = xEventGroupCreate();
    if (job->idle_event == NULL) {
        return false;
    }
    xEventGroupSetBits(job->idle_event, IWP_JOB_EV_IDLE);

    if (xTaskCreate(iwp_job_task, name, stack_words, job, priority, &job->task) != pdPASS) {
        vEventGroupDelete(job->idle_event);
        return false;
    }
    s_jobs[s_job_count++] = job;
    return true;
}

bool iwp_job_start(iwp_job_t *job, void *arg)
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
        xEventGroupClearBits(job->idle_event, IWP_JOB_EV_IDLE);
        xTaskNotify(job->task, IWP_JOB_NOTIFY_START, eSetBits);
    }
    return ok;
}

void iwp_job_request_stop(iwp_job_t *job)
{
    if (job->running) {
        job->stop_requested = true;
        xTaskNotify(job->task, IWP_JOB_NOTIFY_STOP, eSetBits);
    }
}

bool iwp_job_stop(iwp_job_t *job, TickType_t timeout)
{
    if (!job->running) {
        return true;
    }
    iwp_job_request_stop(job);
    EventBits_t ev = xEventGroupWaitBits(job->idle_event, IWP_JOB_EV_IDLE,
                                         pdFALSE, pdFALSE, timeout);
    return (ev & IWP_JOB_EV_IDLE) != 0;
}

bool iwp_job_is_running(const iwp_job_t *job)
{
    return job->running;
}

void iwp_job_request_stop_all(void)
{
    for (uint8_t i = 0; i < s_job_count; i++) {
        iwp_job_request_stop(s_jobs[i]);
    }
}

bool iwp_job_stop_all(TickType_t timeout)
{
    TickType_t start = xTaskGetTickCount();
    bool all = true;

    iwp_job_request_stop_all();
    for (uint8_t i = 0; i < s_job_count; i++) {
        TickType_t used = xTaskGetTickCount() - start;
        TickType_t left = (used >= timeout) ? 0 : (timeout - used);
        if (!iwp_job_stop(s_jobs[i], left)) {
            all = false;
        }
    }
    return all;
}

uint32_t iwp_job_wait(iwp_job_t *job, TickType_t timeout)
{
    (void)job;
    uint32_t bits = 0;
    if (xTaskNotifyWait(0, 0xFFFFFFFFu, &bits, timeout) != pdTRUE) {
        return 0;
    }
    return bits & IWP_JOB_USER_BITS_MASK;
}

void iwp_job_notify(iwp_job_t *job, uint32_t bits)
{
    xTaskNotify(job->task, bits & IWP_JOB_USER_BITS_MASK, eSetBits);
}

void iwp_job_notify_from_isr(iwp_job_t *job, uint32_t bits)
{
    BaseType_t woken = pdFALSE;
    xTaskNotifyFromISR(job->task, bits & IWP_JOB_USER_BITS_MASK, eSetBits, &woken);
    portYIELD_FROM_ISR(woken);
}
