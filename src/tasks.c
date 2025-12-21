/* src/tasks.c */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

/*
 * WorkerTask - Her görev için oluşturulan worker
 * 
 * Bu görev scheduler tarafından yönetilir.
 * Gerçek iş simülasyonu scheduler içinde yapılır.
 */
void WorkerTask(void *pvParameters) {
    TaskInfo *taskInfo = (TaskInfo *)pvParameters;
    
    if (taskInfo == NULL) {
        vTaskDelete(NULL);
        return;
    }
    
    /* Görev yaşam döngüsü */
    for (;;) {
        /* Scheduler bu görevi askıya alır/devam ettirir */
        vTaskDelay(pdMS_TO_TICKS(100));
        
        /* Görev tamamlandıysa çık */
        if (taskInfo->state == TASK_FINISHED) {
            break;
        }
    }
    
    /* Görev siliniyor */
    vTaskDelete(NULL);
}