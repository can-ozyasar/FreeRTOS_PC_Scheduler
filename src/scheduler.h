/* src/scheduler.h */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// Renk Kodları (Terminal Çıktısı İçin)
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
    int priority;           // 0: RT, 1, 2, 3: User
    int burstTime;
    int remainingTime;
    int isFinished;
    int hasStarted;         // İlk kez mi çalışıyor?
    TaskHandle_t handle;
    const char *color;
    struct TaskInfo* next;
} TaskInfo;

typedef struct {
    TaskInfo* head;
    TaskInfo* tail;
    int count;
} Queue;

// Fonksiyonlar
void SchedulerInit(const char* filename);
void SchedulerTask(void *pvParameters);
void WorkerTask(void *pvParameters);

#endif