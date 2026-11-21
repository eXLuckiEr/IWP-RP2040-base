#ifndef JOB_H
#define JOB_H

/* ===========================================================================
 *  job — фоновая задача, запускаемая и останавливаемая командой.
 *
 *  Идея: SCPI-обработчик НИКОГДА не выполняет долгую работу сам. Он вызывает
 *  job_start() и сразу возвращает управление, поэтому SCPI-задача продолжает
 *  читать поток команд (в том числе команду остановки).
 *
 *  Пример (см. app/adc_dump.c):
 *
 *      static job_t my_job;
 *
 *      static void my_job_fn(job_t *job, void *arg) {
 *          while (!job_stop_requested(job)) {
 *              do_one_step();
 *              job_wait(job, pdMS_TO_TICKS(100));   // проснётся раньше по STOP
 *          }
 *      }
 *      job_init(&my_job, "my", my_job_fn, STACK_JOB_DEFAULT, PRIO_JOB);
 *      ...
 *      job_start(&my_job, NULL);     // из SCPI-колбэка
 *      job_stop(&my_job, pdMS_TO_TICKS(500));
 *
 *  Задача создаётся один раз при job_init() и «паркуется» до job_start().
 * ======================================================================== */

#include <stdbool.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>
#include <event_groups.h>

#define JOB_MAX_JOBS        4
/* Биты уведомлений, доступные пользователю (job_notify / job_notify_from_isr) */
#define JOB_USER_BITS_MASK  0x3FFFFFFFu

typedef struct job_s job_t;
typedef void (*job_fn_t)(job_t *job, void *arg);

struct job_s {
    const char       *name;
    job_fn_t          fn;
    void             *arg;
    TaskHandle_t      task;
    EventGroupHandle_t idle_event;
    volatile bool     running;
    volatile bool     stop_requested;
};

/* Создать (запаркованную) задачу. false — не хватило памяти или слотов. */
bool job_init(job_t *job, const char *name, job_fn_t fn,
              uint16_t stack_words, UBaseType_t priority);

/* Запустить. false, если уже выполняется. Не блокирует. */
bool job_start(job_t *job, void *arg);

/* Попросить остановиться (не блокирует). */
void job_request_stop(job_t *job);

/* Попросить остановиться и дождаться завершения. true — job уже не работает. */
bool job_stop(job_t *job, TickType_t timeout);

bool job_is_running(const job_t *job);

/* Остановить все job (ABORt, *RST, потеря USB). */
void job_request_stop_all(void);
bool job_stop_all(TickType_t timeout);

/* ---- вызывается ИЗ тела job -------------------------------------------- */
static inline bool job_stop_requested(const job_t *job) { return job->stop_requested; }

/* Ждать событий/таймаута. Возвращает набор пользовательских битов (0 при
 * таймауте или при STOP). После возврата проверяйте job_stop_requested(). */
uint32_t job_wait(job_t *job, TickType_t timeout);

/* Разбудить job пользовательскими битами (из задачи / из ISR). */
void job_notify(job_t *job, uint32_t bits);
void job_notify_from_isr(job_t *job, uint32_t bits);

#endif /* JOB_H */
