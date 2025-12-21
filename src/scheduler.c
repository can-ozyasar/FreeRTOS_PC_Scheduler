/* src/scheduler.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

/* Global Kuyruklar */
Queue rtQueue = {NULL, NULL, 0};
Queue feedbackQ1 = {NULL, NULL, 0};
Queue feedbackQ2 = {NULL, NULL, 0};
Queue feedbackQ3 = {NULL, NULL, 0};

/* Bekleyen Görevler (henüz varmamış) */
static TaskInfo *allTasks[MAX_TASKS];
static int totalTaskCount = 0;

int globalTime = 0;

/* Renk Dizisi */
static const char *colorPalette[] = {
    COLOR_RED, COLOR_GREEN, COLOR_YELLOW, 
    COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN
};
static const int colorCount = 6;

/* ================================================================
   KUYRUK FONKSİYONLARI
   ================================================================ */

void enqueue(Queue *q, TaskInfo *t) {
    if (q == NULL || t == NULL) return;
    
    t->next = NULL;
    
    if (q->tail == NULL) {
        q->head = t;
        q->tail = t;
    } else {
        q->tail->next = t;
        q->tail = t;
    }
    q->count++;
}

TaskInfo* dequeue(Queue *q) {
    if (q == NULL || q->head == NULL) return NULL;
    
    TaskInfo *t = q->head;
    q->head = t->next;
    
    if (q->head == NULL) {
        q->tail = NULL;
    }
    
    q->count--;
    t->next = NULL;
    return t;
}

void removeFromQueue(Queue *q, TaskInfo *t) {
    if (q == NULL || q->head == NULL || t == NULL) return;
    
    /* Başta ise */
    if (q->head == t) {
        dequeue(q);
        return;
    }
    
    /* Arada veya sonda ise */
    TaskInfo *prev = q->head;
    while (prev->next != NULL && prev->next != t) {
        prev = prev->next;
    }
    
    if (prev->next == t) {
        prev->next = t->next;
        if (q->tail == t) {
            q->tail = prev;
        }
        q->count--;
        t->next = NULL;
    }
}

/* Round-robin için: Görevi kuyruğun sonuna taşı */
void moveToBack(Queue *q, TaskInfo *t) {
    if (q == NULL || t == NULL || q->count <= 1) return;
    
    removeFromQueue(q, t);
    enqueue(q, t);
}

/* ================================================================
   YARDIMCI FONKSİYONLAR
   ================================================================ */

const char* getRandomColor(int id) {
    return colorPalette[id % colorCount];
}

Queue* getQueueByPriority(int priority) {
    switch (priority) {
        case PRIORITY_RT:   return &rtQueue;
        case PRIORITY_HIGH: return &feedbackQ1;
        case PRIORITY_MED:  return &feedbackQ2;
        default:            return &feedbackQ3;  /* 3 ve üzeri */
    }
}

/* En yüksek öncelikli görevi getir (kuyruktan çıkarmadan) */
TaskInfo* peekNextTask(void) {
    if (rtQueue.head != NULL) return rtQueue.head;
    if (feedbackQ1.head != NULL) return feedbackQ1.head;
    if (feedbackQ2.head != NULL) return feedbackQ2.head;
    if (feedbackQ3.head != NULL) return feedbackQ3.head;
    return NULL;
}

/* En yüksek öncelikli görevi al (kuyruktan çıkar) */
TaskInfo* getNextTask(void) {
    if (rtQueue.head != NULL) return dequeue(&rtQueue);
    if (feedbackQ1.head != NULL) return dequeue(&feedbackQ1);
    if (feedbackQ2.head != NULL) return dequeue(&feedbackQ2);
    if (feedbackQ3.head != NULL) return dequeue(&feedbackQ3);
    return NULL;
}

int allQueuesEmpty(void) {
    return (rtQueue.count == 0 && 
            feedbackQ1.count == 0 && 
            feedbackQ2.count == 0 && 
            feedbackQ3.count == 0);
}

/* Bekleyen görev var mı? */
static int hasPendingTasks(void) {
    for (int i = 0; i < totalTaskCount; i++) {
        if (allTasks[i] != NULL && allTasks[i]->state == TASK_WAITING) {
            return 1;
        }
    }
    return 0;
}

/* ================================================================
   GÖREV YÖNETİMİ
   ================================================================ */

/* Varış zamanı gelen görevleri kuyruğa ekle */
static void processArrivals(void) {
    for (int i = 0; i < totalTaskCount; i++) {
        TaskInfo *t = allTasks[i];
        if (t != NULL && t->state == TASK_WAITING && t->arrivalTime == globalTime) {
            t->state = TASK_READY;
            
            /* Önceliğe göre uygun kuyruğa ekle */
            Queue *q = getQueueByPriority(t->originalPriority);
            t->currentPriority = t->originalPriority;
            enqueue(q, t);
            
           // printf("%s%.4f sn\tgörev geldi\t\t(id:%04d\töncelik:%d\tsüre:%d sn)%s\n",
                   //t->color, (double)globalTime, t->id,
                   //t->currentPriority, t->burstTime, COLOR_RESET);
        }
    }
}

/* Görevi askıya al ve önceliği düşür */
static void suspendAndDemote(TaskInfo *t) {
    if (t == NULL) return;
    
    t->state = TASK_SUSPENDED;
    
    /* Önceliği düşür (maksimum 3) */
    if (t->currentPriority < PRIORITY_LOW) {
        t->currentPriority++;
    }
    
    printf("%s%.4f sn\tproses askıda\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
           t->color, (double)globalTime, t->id,
           t->currentPriority, t->remainingTime, COLOR_RESET);
    
    /* Yeni öncelik kuyruğuna ekle */
    Queue *q = getQueueByPriority(t->currentPriority);
    enqueue(q, t);
}

/* Görevi sonlandır */
static void terminateTask(TaskInfo *t, const char *reason) {
    if (t == NULL) return;
    
    t->state = TASK_FINISHED;
    
    printf("%s%.4f sn\tproses %s\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
           t->color, (double)globalTime, reason, t->id,
           t->currentPriority, t->remainingTime, COLOR_RESET);
    
    if (t->handle != NULL) {
        vTaskDelete(t->handle);
        t->handle = NULL;
    }
}

/* ================================================================
   ZAMANLAYICI BAŞLATMA
   ================================================================ */

void SchedulerInit(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Hata: %s dosyası açılamadı!\n", filename);
        exit(1);
    }
    
    printf("\n========================================\n");
    printf("  FreeRTOS Görev Sıralayıcı Simülasyonu\n");
    printf("  4 Seviyeli Öncelikli Görevlendirici\n");
    printf("========================================\n\n");
    
    char line[256];
    int arrival, priority, burst;
    
    while (fgets(line, sizeof(line), file)) {
        /* Boş satırları atla */
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#') continue;
        
        if (sscanf(line, "%d, %d, %d", &arrival, &priority, &burst) == 3) {
            TaskInfo *t = (TaskInfo *)malloc(sizeof(TaskInfo));
            if (t == NULL) continue;
            
            t->id = totalTaskCount;
            t->arrivalTime = arrival;
            t->originalPriority = priority;
            t->currentPriority = priority;
            t->burstTime = burst;
            t->remainingTime = burst;
            t->executedTime = 0;
            t->state = TASK_WAITING;
            t->handle = NULL;
            t->color = getRandomColor(totalTaskCount);
            t->next = NULL;
            
            /* Worker task oluştur (askıda başlar) */
            xTaskCreate(WorkerTask, "Worker", configMINIMAL_STACK_SIZE, 
                       t, 1, &t->handle);
            vTaskSuspend(t->handle);
            
            allTasks[totalTaskCount++] = t;
            
            printf("Görev yüklendi: id=%d, varış=%d, öncelik=%d, süre=%d\n",
                   t->id, arrival, priority, burst);
        }
    }
    
    fclose(file);
    
    printf("\nToplam %d görev yüklendi.\n", totalTaskCount);
    printf("Simülasyon başlıyor...\n\n");
    
    /* Ana zamanlayıcı görevini oluştur */
    xTaskCreate(SchedulerTask, "Scheduler", configMINIMAL_STACK_SIZE * 4, 
               NULL, configMAX_PRIORITIES - 1, NULL);
}

/* ================================================================
   ANA ZAMANLAYICI GÖREVİ
   ================================================================ */

void SchedulerTask(void *pvParameters) {
    TaskInfo *runningTask = NULL;
    int rtRunning = 0;  /* RT görevi çalışıyor mu? */
    
    for (;;) {
        /* ========== 1. VARIŞ ZAMANLARINI KONTROL ET ========== */
        processArrivals();
        
        /* ========== 2. RT PREEMPTION KONTROLÜ ========== */
        /* Yeni RT görevi geldiyse ve kullanıcı görevi çalışıyorsa */
        if (rtQueue.head != NULL && runningTask != NULL && 
            runningTask->currentPriority > PRIORITY_RT && !rtRunning) {
            
            /* Kullanıcı görevini askıya al */
            suspendAndDemote(runningTask);
            runningTask = NULL;
        }
        
        /* ========== 3. ÇALIŞACAK GÖREVİ SEÇ ========== */
        if (runningTask == NULL) {
            runningTask = getNextTask();
            
            if (runningTask != NULL) {
                rtRunning = (runningTask->currentPriority == PRIORITY_RT);
                runningTask->state = TASK_RUNNING;
                
                /* Başladı mesajı */
                printf("%s%.4f sn\tproses başladı\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       runningTask->color, (double)globalTime, runningTask->id,
                       runningTask->currentPriority, runningTask->remainingTime, COLOR_RESET);
            }
        }
        
        /* ========== 4. GÖREV ÇALIŞTIR ========== */
        if (runningTask != NULL) {
            /* Zaten başladı mesajı yukarıda yazdırıldı, şimdi çalıştır */
            
            /* 1 kuantum (1 saniye) çalıştır */
            runningTask->remainingTime--;
            runningTask->executedTime++;
            
            /* ========== 5. GÖREV BİTTİ Mİ? ========== */
            if (runningTask->remainingTime <= 0) {
                globalTime++;
                terminateTask(runningTask, "sonlandı\t");
                runningTask = NULL;
                rtRunning = 0;
            }
            /* ========== 6. TIMEOUT KONTROLÜ (20 sn) ========== */
            else if (runningTask->executedTime >= AUTO_TERMINATE) {
                globalTime++;
                terminateTask(runningTask, "zaman aşımı");
                runningTask = NULL;
                rtRunning = 0;
            }
            /* ========== 7. RT GÖREVİ: KESİNTİSİZ DEVAM ========== */
            else if (runningTask->currentPriority == PRIORITY_RT) {
                globalTime++;
                /* RT devam eder, yürütülüyor mesajı */
                printf("%s%.4f sn\tproses yürütülüyor\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       runningTask->color, (double)globalTime, runningTask->id,
                       runningTask->currentPriority, runningTask->remainingTime, COLOR_RESET);
            }
            /* ========== 8. KULLANICI GÖREVİ: FEEDBACK/RR ========== */
            else {
                globalTime++;
                
                /* Daha yüksek öncelikli görev var mı kontrol et */
                TaskInfo *nextHigher = peekNextTask();
                int needPreempt = 0;
                
                /* RT kuyruğunda görev varsa preempt */
                if (rtQueue.head != NULL) {
                    needPreempt = 1;
                }
                /* Daha yüksek öncelikli feedback kuyruğunda görev varsa */
                else if (nextHigher != NULL && 
                         nextHigher->currentPriority < runningTask->currentPriority) {
                    needPreempt = 1;
                }
                /* Aynı kuyrukta birden fazla görev varsa (Round Robin) */
                else {
                    Queue *currentQ = getQueueByPriority(runningTask->currentPriority);
                    if (currentQ->count > 0) {
                        needPreempt = 1;
                    }
                }
                
                if (needPreempt) {
                    /* Askıya al ve öncelik düşür */
                    suspendAndDemote(runningTask);
                    runningTask = NULL;
                    rtRunning = 0;
                } else {
                    /* Devam edebilir */
                    printf("%s%.4f sn\tproses yürütülüyor\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                           runningTask->color, (double)globalTime, runningTask->id,
                           runningTask->currentPriority, runningTask->remainingTime, COLOR_RESET);
                }
            }
        } else {
            /* Çalışan görev yok */
            globalTime++;
        }
        
        /* ========== 9. SİMÜLASYON BİTTİ Mİ? ========== */
        if (allQueuesEmpty() && !hasPendingTasks() && runningTask == NULL) {
            printf("\n========================================\n");
            printf("  Simülasyon Tamamlandı!\n");
            printf("  Toplam Süre: %d saniye\n", globalTime);
            printf("========================================\n");
            
            vTaskDelay(pdMS_TO_TICKS(500));
            exit(0);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));  /* 1 saniye bekle */
    }
}