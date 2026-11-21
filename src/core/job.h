/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef IWP_JOB_H
#define IWP_JOB_H

/* ===========================================================================
 *  job — фоновая задача, запускаемая и останавливаемая командой.
 *
 *  Идея: SCPI-обработчик НИКОГДА не выполняет долгую работу сам. Он вызывает
 *  iwp_job_start() и сразу возвращает управление, поэтому SCPI-задача продолжает
 *  читать поток команд (в том числе команду остановки).
 *
 *  Пример (см. app/adc_dump.c):
 *
 *      static iwp_job_t my_job;
 *
 *      static void my_job_fn(iwp_job_t *job, void *arg) {
 *          while (!iwp_job_stop_requested(job)) {
 *              do_one_step();
 *              iwp_job_wait(job, pdMS_TO_TICKS(100));   // проснётся раньше по STOP
 *          }
 *      }
 *      iwp_job_init(&my_job, "my", my_job_fn, STACK_JOB_DEFAULT, PRIO_JOB);
 *      ...
 *      iwp_job_start(&my_job, NULL);     // из SCPI-колбэка
 *      iwp_job_stop(&my_job, pdMS_TO_TICKS(500));
 *
 *  Задача создаётся один раз при iwp_job_init() и «паркуется» до iwp_job_start().
 * ======================================================================== */

#include <stdbool.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>
#include <event_groups.h>

#define IWP_JOB_MAX_JOBS        4
/* Биты уведомлений, доступные пользователю (iwp_job_notify / iwp_job_notify_from_isr) */
#define IWP_JOB_USER_BITS_MASK  0x3FFFFFFFu

typedef struct iwp_job_s iwp_job_t;
typedef void (*iwp_job_fn_t)(iwp_job_t *job, void *arg);

struct iwp_job_s {
    const char       *name;
    iwp_job_fn_t          fn;
    void             *arg;
    TaskHandle_t      task;
    EventGroupHandle_t idle_event;
    volatile bool     running;
    volatile bool     stop_requested;
};

/* Создать (запаркованную) задачу. false — не хватило памяти или слотов. */
bool iwp_job_init(iwp_job_t *job, const char *name, iwp_job_fn_t fn,
              uint16_t stack_words, UBaseType_t priority);

/* Запустить. false, если уже выполняется. Не блокирует. */
bool iwp_job_start(iwp_job_t *job, void *arg);

/* Попросить остановиться (не блокирует). */
void iwp_job_request_stop(iwp_job_t *job);

/* Попросить остановиться и дождаться завершения. true — job уже не работает. */
bool iwp_job_stop(iwp_job_t *job, TickType_t timeout);

bool iwp_job_is_running(const iwp_job_t *job);

/* Остановить все job (ABORt, *RST, потеря USB). */
void iwp_job_request_stop_all(void);
bool iwp_job_stop_all(TickType_t timeout);

/* ---- вызывается ИЗ тела job -------------------------------------------- */
static inline bool iwp_job_stop_requested(const iwp_job_t *job) { return job->stop_requested; }

/* Ждать событий/таймаута. Возвращает набор пользовательских битов (0 при
 * таймауте или при STOP). После возврата проверяйте iwp_job_stop_requested(). */
uint32_t iwp_job_wait(iwp_job_t *job, TickType_t timeout);

/* Разбудить job пользовательскими битами (из задачи / из ISR). */
void iwp_job_notify(iwp_job_t *job, uint32_t bits);
void iwp_job_notify_from_isr(iwp_job_t *job, uint32_t bits);

#endif /* IWP_JOB_H */
