/* src/scheduler.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

/* Global Kuyruklar */
Queue qRT  = {NULL, NULL, 0};   /* Real-Time kuyruk */
Queue qFB1 = {NULL, NULL, 0};   /* Feedback seviye 1 */
Queue qFB2 = {NULL, NULL, 0};   /* Feedback seviye 2 */
Queue qFB3 = {NULL, NULL, 0};   /* Feedback seviye 3 (Round-Robin) */

/* Bekleyen görevler (henüz varmamış) */
TaskInfo *pendingTasks[MAX_TASKS];
int pendingCount = 0;

/* Global zaman sayacı */
int globalTime = 0;

/* Yazdırma için mutex */
SemaphoreHandle_t printMutex = NULL;

/* Renk havuzu */
static const char *colorPool[] = {
    COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, 
    COLOR_MAGENTA, COLOR_CYAN, COLOR_BRIGHT_RED, COLOR_BRIGHT_GREEN,
    COLOR_BRIGHT_YELLOW, COLOR_BRIGHT_BLUE, COLOR_BRIGHT_MAGENTA, COLOR_BRIGHT_CYAN
};
static const int colorCount = 12;

/*******************************************************************************
 * KUYRUK FONKSİYONLARI
 ******************************************************************************/

/* Kuyruğun sonuna ekle (FIFO) */
void enqueue(Queue* q, TaskInfo* t) {
    if (q == NULL || t == NULL) return;
    
    t->next = NULL;
    
    if (q->tail != NULL) {
        q->tail->next = t;
    }
    q->tail = t;
    
    if (q->head == NULL) {
        q->head = t;
    }
    q->count++;
}

/* Kuyruğun başından çıkar */
TaskInfo* dequeue(Queue* q) {
    if (q == NULL || q->head == NULL) return NULL;
    
    TaskInfo* temp = q->head;
    q->head = q->head->next;
    
    if (q->head == NULL) {
        q->tail = NULL;
    }
    q->count--;
    temp->next = NULL;
    
    return temp;
}

/* Belirli bir elemanı kuyruktan çıkar */
void removeFromQueue(Queue* q, TaskInfo* t) {
    if (q == NULL || q->head == NULL || t == NULL) return;
    
    /* Başta ise */
    if (q->head == t) {
        dequeue(q);
        return;
    }
    
    /* Ortada veya sonda ara */
    TaskInfo* current = q->head;
    while (current->next != NULL && current->next != t) {
        current = current->next;
    }
    
    if (current->next == t) {
        current->next = t->next;
        if (q->tail == t) {
            q->tail = current;
        }
        q->count--;
        t->next = NULL;
    }
}

/* Elemanı kuyruğun sonuna taşı (Round-Robin için) */
void moveToEndOfQueue(Queue* q, TaskInfo* t) {
    if (q == NULL || t == NULL || q->count <= 1) return;
    
    removeFromQueue(q, t);
    enqueue(q, t);
}

/*******************************************************************************
 * YARDIMCI FONKSİYONLAR
 ******************************************************************************/

/* Önceliğe göre uygun kuyruğu döndür */
Queue* getQueueForPriority(int priority) {
    switch (priority) {
        case PRIORITY_RT:   return &qRT;
        case PRIORITY_HIGH: return &qFB1;
        case PRIORITY_MED:  return &qFB2;
        default:            return &qFB3;  /* 3 ve üzeri */
    }
}

/* En yüksek öncelikli hazır görevi döndür */
TaskInfo* getHighestPriorityTask(void) {
    if (qRT.head != NULL)  return qRT.head;
    if (qFB1.head != NULL) return qFB1.head;
    if (qFB2.head != NULL) return qFB2.head;
    if (qFB3.head != NULL) return qFB3.head;
    return NULL;
}

/* Tüm kuyruklar boş mu? */
int allQueuesEmpty(void) {
    return (qRT.count == 0 && qFB1.count == 0 && 
            qFB2.count == 0 && qFB3.count == 0);
}

/* Bekleyen görev var mı? */
static int hasPendingTasks(void) {
    for (int i = 0; i < pendingCount; i++) {
        if (pendingTasks[i] != NULL && 
            pendingTasks[i]->state == TASK_STATE_PENDING) {
            return 1;
        }
    }
    return 0;
}

/* Görev için rastgele renk al */
const char* getRandomColor(int id) {
    return colorPool[id % colorCount];
}

/* Görevi tüm kullanıcı kuyruklarından çıkar */
static void removeFromAllUserQueues(TaskInfo* t) {
    removeFromQueue(&qFB1, t);
    removeFromQueue(&qFB2, t);
    removeFromQueue(&qFB3, t);
}

/*******************************************************************************
 * DURUM YAZICI FONKSİYONLARI
 ******************************************************************************/

static void printTaskStarted(TaskInfo* t) {
    printf("%s%d.0000 sn\tproses başladı\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
           t->color, globalTime, t->id, t->currentPriority, t->remainingTime, COLOR_RESET);
}

static void printTaskRunning(TaskInfo* t) {
    printf("%s%d.0000 sn\tproses yürütülüyor\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
           t->color, globalTime, t->id, t->currentPriority, t->remainingTime, COLOR_RESET);
}

static void printTaskSuspended(TaskInfo* t) {
    printf("%s%d.0000 sn\tproses askıda\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
           t->color, globalTime, t->id, t->currentPriority, t->remainingTime, COLOR_RESET);
}

static void printTaskResumed(TaskInfo* t) {
    printf("%s%d.0000 sn\tproses devam ediyor\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
           t->color, globalTime, t->id, t->currentPriority, t->remainingTime, COLOR_RESET);
}

static void printTaskFinished(TaskInfo* t) {
    printf("%s%d.0000 sn\tproses sonlandı\t\t(id:%04d\töncelik:%d\tkalan süre:0 sn)%s\n",
           t->color, globalTime, t->id, t->currentPriority, COLOR_RESET);
}

static void printTaskTimeout(TaskInfo* t) {
    printf("%s%d.0000 sn\tproses zamanaşımı\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
           t->color, globalTime, t->id, t->currentPriority, t->remainingTime, COLOR_RESET);
}

/*******************************************************************************
 * ZAMANLAYICI BAŞLATMA
 ******************************************************************************/

void SchedulerInit(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Dosya açılamadı");
        exit(1);
    }
    
    printf("\n========== FreeRTOS 4 Seviyeli Öncelikli Görev Sıralayıcı ==========\n");
    printf("Dosya: %s\n", filename);
    printf("====================================================================\n\n");
    
    char line[256];
    int arrival, priority, burst;
    int taskId = 0;
    
    /* Dosyayı satır satır oku */
    while (fgets(line, sizeof(line), file)) {
        /* Boş satırları atla */
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#') continue;
        
        if (sscanf(line, "%d, %d, %d", &arrival, &priority, &burst) == 3) {
            TaskInfo* t = (TaskInfo*)malloc(sizeof(TaskInfo));
            if (t == NULL) {
                perror("Bellek ayırma hatası");
                exit(1);
            }
            
            t->id = taskId;
            t->arrivalTime = arrival;
            t->originalPriority = priority;
            t->currentPriority = priority;
            t->burstTime = burst;
            t->remainingTime = burst;
            t->executionTime = 0;
            t->state = TASK_STATE_PENDING;
            t->handle = NULL;
            t->color = getRandomColor(taskId);
            t->next = NULL;
            
            /* Worker task oluştur ve askıya al */
            char taskName[16];
            snprintf(taskName, sizeof(taskName), "Task%d", taskId);
            
            BaseType_t result = xTaskCreate(
                WorkerTask,
                taskName,
                configMINIMAL_STACK_SIZE,
                (void*)t,
                1,
                &t->handle
            );
            
            if (result == pdPASS && t->handle != NULL) {
                vTaskSuspend(t->handle);
            }
            
            pendingTasks[pendingCount++] = t;
            
            printf("Görev yüklendi: ID=%d, Varış=%d, Öncelik=%d, Süre=%d\n",
                   taskId, arrival, priority, burst);
            
            taskId++;
            
            if (pendingCount >= MAX_TASKS) {
                printf("Uyarı: Maksimum görev sayısına ulaşıldı!\n");
                break;
            }
        }
    }
    
    fclose(file);
    
    printf("\nToplam %d görev yüklendi.\n", pendingCount);
    printf("====================================================================\n\n");
    
    /* Scheduler task'ı oluştur */
    xTaskCreate(
        SchedulerTask,
        "Scheduler",
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        configMAX_PRIORITIES - 1,
        NULL
    );
}

/*******************************************************************************
 * ANA ZAMANLAYICI GÖREVİ
 ******************************************************************************/

void SchedulerTask(void *pvParameters) {
    (void)pvParameters;
    
    TaskInfo *runningTask = NULL;
    int runningTaskFirstTick = 0;  /* İlk tick mi? */
    
    for (;;) {
        /*******************************************************************
         * ADIM 1: YENİ VARAN GÖREVLERİ KUYRUKLARA EKLE
         *******************************************************************/
        for (int i = 0; i < pendingCount; i++) {
            TaskInfo* t = pendingTasks[i];
            
            if (t != NULL && t->state == TASK_STATE_PENDING && 
                t->arrivalTime == globalTime) {
                
                t->state = TASK_STATE_READY;
                
                /* Önceliğe göre uygun kuyruğa ekle */
                Queue* targetQueue = getQueueForPriority(t->currentPriority);
                enqueue(targetQueue, t);
            }
        }
        
        /*******************************************************************
         * ADIM 2: PREEMPTION KONTROLÜ (RT görev geldi mi?)
         *******************************************************************/
        if (runningTask != NULL && runningTask->state == TASK_STATE_RUNNING) {
            /* Eğer çalışan görev kullanıcı görevi ise ve RT kuyruğunda görev varsa */
            if (runningTask->currentPriority > PRIORITY_RT && qRT.count > 0) {
                /* Kullanıcı görevini askıya al */
                runningTask->state = TASK_STATE_SUSPENDED;
                
                /* Önceliği düşür (değer artar) - sadece feedback kuyruklarında */
                if (runningTask->currentPriority < PRIORITY_LOW) {
                    runningTask->currentPriority++;
                }
                
                printTaskSuspended(runningTask);
                
                /* Mevcut kuyruktan çıkar ve yeni kuyruğa ekle */
                removeFromAllUserQueues(runningTask);
                Queue* newQueue = getQueueForPriority(runningTask->currentPriority);
                enqueue(newQueue, runningTask);
                
                runningTask = NULL;
            }
        }
        
        /*******************************************************************
         * ADIM 3: ÇALIŞTIRILACAK GÖREVİ SEÇ
         *******************************************************************/
        if (runningTask == NULL) {
            TaskInfo* nextTask = getHighestPriorityTask();
            
            if (nextTask != NULL) {
                /* Kuyruktan çıkar */
                Queue* srcQueue = getQueueForPriority(nextTask->currentPriority);
                dequeue(srcQueue);
                
                runningTask = nextTask;
                runningTaskFirstTick = 1;
                
                if (runningTask->state == TASK_STATE_SUSPENDED) {
                    /* Askıdan devam ediyor */
                    runningTask->state = TASK_STATE_RUNNING;
                    printTaskResumed(runningTask);
                } else {
                    /* İlk kez başlıyor */
                    runningTask->state = TASK_STATE_RUNNING;
                    printTaskStarted(runningTask);
                }
            }
        }
        
        /*******************************************************************
         * ADIM 4: GÖREVİ ÇALIŞTIR (1 kuantum)
         *******************************************************************/
        if (runningTask != NULL && runningTask->state == TASK_STATE_RUNNING) {
            /* Bu tick'te çalıştır */
            if (!runningTaskFirstTick) {
                printTaskRunning(runningTask);
            }
            runningTaskFirstTick = 0;
            
            /* Zamanları güncelle */
            runningTask->remainingTime--;
            runningTask->executionTime++;
            
            /***************************************************************
             * ADIM 5: GÖREV BİTTİ Mİ?
             ***************************************************************/
            if (runningTask->remainingTime == 0) {
                /* Görev tamamlandı */
                globalTime++;  /* Zamanı ilerlet (mesaj için) */
                printTaskFinished(runningTask);
                
                runningTask->state = TASK_STATE_FINISHED;
                
                if (runningTask->handle != NULL) {
                    vTaskDelete(runningTask->handle);
                    runningTask->handle = NULL;
                }
                
                runningTask = NULL;
                
                /* Tick delay yap ve döngüye devam */
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            
            /***************************************************************
             * ADIM 6: TIMEOUT KONTROLÜ (20 saniye)
             ***************************************************************/
            if (runningTask->executionTime >= MAX_TIMEOUT) {
                globalTime++;
                printTaskTimeout(runningTask);
                
                runningTask->state = TASK_STATE_FINISHED;
                
                if (runningTask->handle != NULL) {
                    vTaskDelete(runningTask->handle);
                    runningTask->handle = NULL;
                }
                
                runningTask = NULL;
                
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            
            /***************************************************************
             * ADIM 7: KUANTUM BİTTİ - PREEMPTION / ROUND-ROBIN KONTROLÜ
             ***************************************************************/
            if (runningTask->currentPriority == PRIORITY_RT) {
                /* RT görevi: Kesintisiz çalışmaya devam eder */
                /* Hiçbir şey yapma, döngü devam edecek */
            }
            else {
                /* Kullanıcı görevi: Feedback mantığı uygula */
                
                /* Daha yüksek öncelikli görev var mı? */
                TaskInfo* nextHigher = getHighestPriorityTask();
                int shouldPreempt = 0;
                
                if (nextHigher != NULL && 
                    nextHigher->currentPriority < runningTask->currentPriority) {
                    shouldPreempt = 1;
                }
                
                /* Aynı seviyede başka görev var mı? (Round-Robin) */
                Queue* currentQueue = getQueueForPriority(runningTask->currentPriority);
                if (currentQueue->count > 0) {
                    shouldPreempt = 1;
                }
                
                if (shouldPreempt) {
                    globalTime++;
                    
                    /* Önceliği düşür */
                    if (runningTask->currentPriority < PRIORITY_LOW) {
                        runningTask->currentPriority++;
                    }
                    
                    runningTask->state = TASK_STATE_SUSPENDED;
                    printTaskSuspended(runningTask);
                    
                    /* Yeni kuyruğa ekle */
                    Queue* newQueue = getQueueForPriority(runningTask->currentPriority);
                    enqueue(newQueue, runningTask);
                    
                    runningTask = NULL;
                    
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    continue;
                }
                /* else: Tek başına, devam eder */
            }
        }
        
        /*******************************************************************
         * ADIM 8: SİMÜLASYON BİTTİ Mİ?
         *******************************************************************/
        if (runningTask == NULL && allQueuesEmpty() && !hasPendingTasks()) {
            printf("\n====================================================================\n");
            printf("Tüm görevler tamamlandı. Simülasyon sona erdi.\n");
            printf("Toplam süre: %d saniye\n", globalTime);
            printf("====================================================================\n");
            
            vTaskDelay(pdMS_TO_TICKS(500));
            exit(0);
        }
        
        /*******************************************************************
         * ADIM 9: ZAMANI İLERLET
         *******************************************************************/
        globalTime++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}