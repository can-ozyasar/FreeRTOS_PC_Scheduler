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

/* Öncelik Seviyeleri */
#define PRIORITY_RT     0   /* Gerçek Zamanlı - En yüksek öncelik */
#define PRIORITY_HIGH   1   /* Kullanıcı - Yüksek */
#define PRIORITY_MED    2   /* Kullanıcı - Orta */
#define PRIORITY_LOW    3   /* Kullanıcı - Düşük */

/* Sabitler */
#define TIME_QUANTUM    1   /* Temel zaman kuantumu (saniye) */
#define MAX_TIMEOUT     20  /* Maksimum görev ömrü (saniye) */
#define MAX_TASKS       100

/* Görev Durumları */
typedef enum {
    TASK_STATE_PENDING,     /* Henüz varmadı */
    TASK_STATE_READY,       /* Hazır, kuyrukta bekliyor */
    TASK_STATE_RUNNING,     /* Çalışıyor */
    TASK_STATE_SUSPENDED,   /* Askıya alındı */
    TASK_STATE_FINISHED     /* Tamamlandı */
} TaskState;

/* Görev Bilgi Yapısı */
typedef struct TaskInfo {
    int id;                     /* Benzersiz görev kimliği */
    int arrivalTime;            /* Varış zamanı */
    int originalPriority;       /* Başlangıç önceliği (dosyadan okunan) */
    int currentPriority;        /* Mevcut öncelik (feedback ile değişir) */
    int burstTime;              /* Toplam görev süresi */
    int remainingTime;          /* Kalan süre */
    int executionTime;          /* Toplam çalışma süresi (timeout için) */
    TaskState state;            /* Görev durumu */
    TaskHandle_t handle;        /* FreeRTOS task handle */
    const char *color;          /* Konsol rengi */
    struct TaskInfo* next;      /* Bağlı liste için sonraki görev */
} TaskInfo;

/* Kuyruk Yapısı */
typedef struct {
    TaskInfo* head;
    TaskInfo* tail;
    int count;
} Queue;

/* Global Kuyruklar */
extern Queue qRT;       /* Priority 0: Real-Time (FCFS) */
extern Queue qFB1;      /* Priority 1: Feedback Level 1 */
extern Queue qFB2;      /* Priority 2: Feedback Level 2 */
extern Queue qFB3;      /* Priority 3: Feedback Level 3 (Round-Robin) */

/* Global Değişkenler */
extern int globalTime;
extern SemaphoreHandle_t printMutex;

/* Fonksiyon Prototipleri */
void SchedulerInit(const char* filename);
void SchedulerTask(void *pvParameters);
void WorkerTask(void *pvParameters);

/* Kuyruk Fonksiyonları */
void enqueue(Queue* q, TaskInfo* t);
TaskInfo* dequeue(Queue* q);
void removeFromQueue(Queue* q, TaskInfo* t);
void moveToEndOfQueue(Queue* q, TaskInfo* t);

/* Yardımcı Fonksiyonlar */
Queue* getQueueForPriority(int priority);
TaskInfo* getHighestPriorityTask(void);
int allQueuesEmpty(void);
const char* getRandomColor(int id);

#endif