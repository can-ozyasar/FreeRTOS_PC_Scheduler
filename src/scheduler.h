/* src/scheduler.h */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* RENK KODLARI */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

/* ÖNCELİK SABİTLERİ */
#define PRIORITY_RT     0
#define PRIORITY_HIGH   1
#define PRIORITY_MED    2
#define PRIORITY_LOW    3
#define MAX_PRIORITY    3 

/* GÖREV BİLGİ YAPISI */
typedef struct TaskInfo {
    int id;
    int arrivalTime;
    int originalPriority;
    int priority;
    int burstTime;
    int remainingTime;
    int waitCounter;
    int isFinished;
    int hasStarted;
    TaskHandle_t handle;
    const char* color;
    struct TaskInfo* next;
} TaskInfo;

/* KUYRUK YAPISI */
typedef struct {
    TaskInfo* head;
    TaskInfo* tail;
    int count;
} Queue;

/* GLOBAL KUYRUKLAR */
extern Queue qRT;
extern Queue qP1;
extern Queue qP2;
extern Queue qP3;

/* FONKSİYON PROTOTİPLERİ */
void SchedulerInit(const char* filename);
void SchedulerTask(void* pvParameters);

#endif