/* src/scheduler.h */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Renk Kodları */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BRIGHT_RED     "\033[91m"
#define COLOR_BRIGHT_GREEN   "\033[92m"
#define COLOR_BRIGHT_YELLOW  "\033[93m"
#define COLOR_BRIGHT_BLUE    "\033[94m"
#define COLOR_BRIGHT_MAGENTA "\033[95m"
#define COLOR_BRIGHT_CYAN    "\033[96m"

/* Sabitler */
#define MAX_TASKS       100
#define TIMEOUT_SECONDS 20

/* Görev Durumları */
typedef enum {
    STATE_PENDING,      /* Henüz varmadı */
    STATE_READY,        /* Kuyrukta bekliyor */
    STATE_RUNNING,      /* Şu an çalışıyor */
    STATE_SUSPENDED,    /* Askıya alındı */
    STATE_FINISHED,     /* Tamamlandı */
    STATE_TIMEOUT       /* Zamanaşımı */
} TaskState;

/* Görev Bilgi Yapısı */
typedef struct TaskInfo {
    int id;
    int arrivalTime;
    int originalPriority;   /* Dosyadan okunan öncelik */
    int currentPriority;    /* Feedback ile değişen öncelik */
    int burstTime;          /* Toplam çalışma süresi */
    int remainingTime;      /* Kalan süre */
    TaskState state;
    TaskHandle_t handle;
    const char *color;
    struct TaskInfo *next;
} TaskInfo;

/* Kuyruk Yapısı */
typedef struct {
    TaskInfo *head;
    TaskInfo *tail;
    int count;
} Queue;

/* Global Değişkenler */
extern Queue rtQueue;       /* Öncelik 0: Real-Time (FCFS) */
extern Queue fbQueue1;      /* Öncelik 1 */
extern Queue fbQueue2;      /* Öncelik 2 */
extern Queue fbQueue3;      /* Öncelik 3+ (Round-Robin) */
extern int globalTime;

/* Fonksiyon Prototipleri */
void SchedulerInit(const char *filename);
void SchedulerTask(void *pvParameters);
void WorkerTask(void *pvParameters);

#endif