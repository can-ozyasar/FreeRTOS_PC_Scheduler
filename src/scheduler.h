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

/* Öncelik Seviyeleri */
#define PRIORITY_RT     0   /* Gerçek Zamanlı - FCFS */
#define PRIORITY_HIGH   1   /* Kullanıcı Yüksek */
#define PRIORITY_MED    2   /* Kullanıcı Orta */
#define PRIORITY_LOW    3   /* Kullanıcı Düşük - Round Robin */

/* Sabitler */
#define MAX_TASKS       100
#define TIME_QUANTUM    1       /* 1 saniye */
#define AUTO_TERMINATE  20      /* 20 saniye sonra otomatik sonlanma */

/* Görev Durumları */
typedef enum {
    TASK_WAITING,       /* Henüz varmadı */
    TASK_READY,         /* Hazır, kuyrukta bekliyor */
    TASK_RUNNING,       /* Çalışıyor */
    TASK_SUSPENDED,     /* Askıya alındı */
    TASK_FINISHED       /* Tamamlandı */
} TaskState;

/* Görev Bilgi Yapısı */
typedef struct TaskInfo {
    int id;
    int arrivalTime;
    int originalPriority;   /* Dosyadan okunan öncelik */
    int currentPriority;    /* Mevcut öncelik (feedback ile değişebilir) */
    int burstTime;          /* Toplam gerekli süre */
    int remainingTime;      /* Kalan süre */
    int executedTime;       /* Toplam çalıştırılan süre (timeout için) */
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
extern Queue rtQueue;       /* Öncelik 0: Gerçek Zamanlı FCFS */
extern Queue feedbackQ1;    /* Öncelik 1: Yüksek */
extern Queue feedbackQ2;    /* Öncelik 2: Orta */
extern Queue feedbackQ3;    /* Öncelik 3: Düşük (Round Robin) */

extern int globalTime;

/* Fonksiyon Prototipleri */
void SchedulerInit(const char *filename);
void SchedulerTask(void *pvParameters);
void WorkerTask(void *pvParameters);

/* Kuyruk İşlemleri */
void enqueue(Queue *q, TaskInfo *t);
TaskInfo* dequeue(Queue *q);
void removeFromQueue(Queue *q, TaskInfo *t);
void moveToBack(Queue *q, TaskInfo *t);

/* Yardımcı Fonksiyonlar */
Queue* getQueueByPriority(int priority);
TaskInfo* getNextTask(void);
int allQueuesEmpty(void);
const char* getRandomColor(int id);

#endif