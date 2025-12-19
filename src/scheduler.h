/* src/scheduler.h */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// Renk Kodları
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

typedef struct TaskInfo {
    int id;
    int arrivalTime;
    int priority;       // 0: RT, 1+: User (Sınır yok, 4, 5 olabilir)
    int burstTime;
    int remainingTime;
    int waitCounter;    // 20sn Kuralı Sayacı
    TaskHandle_t handle;
    const char* color;
    int isFinished;
    int hasStarted;
    struct TaskInfo* next;
} TaskInfo;

typedef struct {
    TaskInfo* head;
    TaskInfo* tail;
    int count;
} Queue;

// Global Değişkenler
extern Queue qRT;  // Priority 0
extern Queue qP1;  // Priority 1
extern Queue qP2;  // Priority 2
extern Queue qP3;  // Priority 3 ve üzeri (Generic Low Priority Queue)

void SchedulerInit(const char* filename);
void SchedulerTask(void *pvParameters);
void WorkerTask(void *pvParameters);

#endif
