/* src/tasks.c */
#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

void WorkerTask(void *pvParameters) {
    // WorkerTask aslında sadece semboliktir. 
    // İşlemler SchedulerTask içinde simüle edilir.
    // Bu task sadece var olup uyur.
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}