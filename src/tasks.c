/* src/tasks.c */
#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

void WorkerTask(void *pvParameters) {
    TaskInfo* t = (TaskInfo*)pvParameters;
    while(1) {
        // İşçi task sadece var olur, yönetimi Scheduler yapar.
        vTaskDelay(pdMS_TO_TICKS(1000));
        if(t && t->isFinished) {
            vTaskDelete(NULL);
        }
    }
}
