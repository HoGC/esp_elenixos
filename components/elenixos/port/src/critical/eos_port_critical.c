#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "eos_port.h"
#include "eos_port_critical.h"

typedef struct eos_sem_t {
    SemaphoreHandle_t handle;
} eos_sem_t;

static portMUX_TYPE s_critical_mux = portMUX_INITIALIZER_UNLOCKED;

eos_critical_ctx_t eos_critical_enter(void)
{
    portENTER_CRITICAL(&s_critical_mux);
    return 0;
}

void eos_critical_leave(eos_critical_ctx_t ctx)
{
    (void)ctx;
    portEXIT_CRITICAL(&s_critical_mux);
}

eos_sem_t *eos_sem_create(uint32_t initial_count, uint32_t max_count)
{
    eos_sem_t *sem = calloc(1, sizeof(*sem));
    if (!sem) {
        return NULL;
    }

    if (max_count <= 1) {
        sem->handle = xSemaphoreCreateMutex();
        if (sem->handle && initial_count == 0) {
            xSemaphoreTake(sem->handle, 0);
        }
    } else {
        sem->handle = xSemaphoreCreateCounting(max_count, initial_count);
    }

    if (!sem->handle) {
        free(sem);
        return NULL;
    }

    return sem;
}

void eos_sem_destroy(eos_sem_t *sem)
{
    if (!sem) {
        return;
    }
    if (sem->handle) {
        vSemaphoreDelete(sem->handle);
    }
    free(sem);
}

bool eos_sem_take(eos_sem_t *sem, uint32_t timeout_ms)
{
    if (!sem || !sem->handle) {
        return false;
    }

    TickType_t ticks = timeout_ms == EOS_TIMEOUT_INFINITE ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(sem->handle, ticks) == pdTRUE;
}

void eos_sem_give(eos_sem_t *sem)
{
    if (!sem || !sem->handle) {
        return;
    }
    xSemaphoreGive(sem->handle);
}
