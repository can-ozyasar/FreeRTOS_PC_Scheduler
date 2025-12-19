/* src/tasks.c */
#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"
#include "tasks.h"

// WorkerTask fonksiyonu burada tanımlanmalı!
// "static" kelimesi OLMAMALI.
void WorkerTask(void* pvParameters) {
    TaskInfo* t = (TaskInfo*)pvParameters;
    
    while (1) {
        // 1 saniye bekle
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Görev bittiyse kendini sil
        if (t != NULL && t->isFinished) {
            vTaskDelete(NULL);
            break;
        }
    }
}