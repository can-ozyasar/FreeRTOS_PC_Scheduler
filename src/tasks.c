/* src/tasks.c */
#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"
#include <stdio.h>

/*******************************************************************************
 * WORKER TASK
 * 
 * Her görev için oluşturulan genel çalışan görev.
 * Scheduler tarafından yönetilir, kendi başına bir iş yapmaz.
 ******************************************************************************/

void WorkerTask(void *pvParameters) {
    TaskInfo *taskInfo = (TaskInfo *)pvParameters;
    
    if (taskInfo == NULL) {
        vTaskDelete(NULL);
        return;
    }
    
    /* Sonsuz döngüde bekle - tüm yönetim Scheduler'da */
    for (;;) {
        /* Scheduler bu görevi yönetir */
        vTaskDelay(pdMS_TO_TICKS(100));
        
        /* Görev bitmiş mi kontrol et */
        if (taskInfo->state == TASK_STATE_FINISHED) {
            break;
        }
    }
    
    /* Görev siliniyor */
    vTaskDelete(NULL);
}