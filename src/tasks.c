/* src/tasks.c */
#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

void WorkerTask(void *pvParameters) {
    TaskInfo *t = (TaskInfo *)pvParameters;
    
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
        
        if (t && (t->state == STATE_FINISHED || t->state == STATE_TIMEOUT)) {
            break;
        }
    }
    
    vTaskDelete(NULL);
}