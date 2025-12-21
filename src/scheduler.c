/* src/scheduler.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

/* Global Kuyruklar */
Queue rtQueue  = {NULL, NULL, 0};
Queue fbQueue1 = {NULL, NULL, 0};
Queue fbQueue2 = {NULL, NULL, 0};
Queue fbQueue3 = {NULL, NULL, 0};

/* Bekleyen görevler listesi */
TaskInfo *allTasks[MAX_TASKS];
int totalTasks = 0;

/* Global zaman */
int globalTime = 0;

/* Renk havuzu */
static const char *colors[] = {
    COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,
    COLOR_MAGENTA, COLOR_CYAN, COLOR_BRIGHT_RED, COLOR_BRIGHT_GREEN,
    COLOR_BRIGHT_YELLOW, COLOR_BRIGHT_BLUE, COLOR_BRIGHT_MAGENTA, COLOR_BRIGHT_CYAN
};
#define NUM_COLORS 12

/*****************************************************************************
 * KUYRUK FONKSİYONLARI
 *****************************************************************************/

void enqueue(Queue *q, TaskInfo *t) {
    if (!q || !t) return;
    t->next = NULL;
    if (q->tail) {
        q->tail->next = t;
    } else {
        q->head = t;
    }
    q->tail = t;
    q->count++;
}

TaskInfo *dequeue(Queue *q) {
    if (!q || !q->head) return NULL;
    TaskInfo *t = q->head;
    q->head = t->next;
    if (!q->head) q->tail = NULL;
    q->count--;
    t->next = NULL;
    return t;
}

void removeFromQueue(Queue *q, TaskInfo *t) {
    if (!q || !q->head || !t) return;
    
    if (q->head == t) {
        dequeue(q);
        return;
    }
    
    TaskInfo *prev = q->head;
    while (prev->next && prev->next != t) {
        prev = prev->next;
    }
    
    if (prev->next == t) {
        prev->next = t->next;
        if (q->tail == t) q->tail = prev;
        q->count--;
        t->next = NULL;
    }
}

/*****************************************************************************
 * YARDIMCI FONKSİYONLAR
 *****************************************************************************/

/* Önceliğe göre kuyruk döndür */
Queue *getQueue(int priority) {
    if (priority == 0) return &rtQueue;
    if (priority == 1) return &fbQueue1;
    if (priority == 2) return &fbQueue2;
    return &fbQueue3;  /* 3 ve üzeri hep burada */
}

/* En yüksek öncelikli görevi al (kuyruktan çıkarmadan) */
TaskInfo *peekHighestPriority(void) {
    if (rtQueue.head)  return rtQueue.head;
    if (fbQueue1.head) return fbQueue1.head;
    if (fbQueue2.head) return fbQueue2.head;
    if (fbQueue3.head) return fbQueue3.head;
    return NULL;
}

/* Tüm kuyruklar boş mu? */
int allQueuesEmpty(void) {
    return (rtQueue.count == 0 && fbQueue1.count == 0 &&
            fbQueue2.count == 0 && fbQueue3.count == 0);
}

/* Bekleyen (henüz varmamış) görev var mı? */
int hasPendingTasks(void) {
    for (int i = 0; i < totalTasks; i++) {
        if (allTasks[i] && allTasks[i]->state == STATE_PENDING) {
            return 1;
        }
    }
    return 0;
}

/* Görevi tüm feedback kuyruklarından çıkar */
void removeFromAllQueues(TaskInfo *t) {
    removeFromQueue(&rtQueue, t);
    removeFromQueue(&fbQueue1, t);
    removeFromQueue(&fbQueue2, t);
    removeFromQueue(&fbQueue3, t);
}

/*****************************************************************************
 * ÇIKTI FONKSİYONLARI
 *****************************************************************************/

void printStarted(TaskInfo *t, int time) {
    printf("%s%d.0000 sn\tproses başladı\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
           t->color, time, t->id, t->currentPriority, t->remainingTime, COLOR_RESET);
}

void printRunning(TaskInfo *t, int time) {
    printf("%s%d.0000 sn\tproses yürütülüyor\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
           t->color, time, t->id, t->currentPriority, t->remainingTime, COLOR_RESET);
}

void printSuspended(TaskInfo *t, int time) {
    printf("%s%d.0000 sn\tproses askıda\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
           t->color, time, t->id, t->currentPriority, t->remainingTime, COLOR_RESET);
}

void printFinished(TaskInfo *t, int time) {
    printf("%s%d.0000 sn \tproses sonlandı\t \t(id:%04d\töncelik:%d\tkalan süre:0 sn)%s\n",
           t->color, time, t->id, t->currentPriority, COLOR_RESET);
}

void printTimeout(TaskInfo *t, int time) {
    printf("%s%d.0000 sn\tproses zamanaşımı\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
           t->color, time, t->id, t->currentPriority, t->remainingTime, COLOR_RESET);
}

/*****************************************************************************
 * TIMEOUT KONTROLÜ
 * Varış zamanından itibaren 20 saniye geçen görevleri sonlandır
 *****************************************************************************/

void checkTimeouts(int currentTime) {
    /* Sadece kullanıcı kuyruklarını kontrol et (RT değil) */
    Queue *queues[] = {&fbQueue1, &fbQueue2, &fbQueue3};
    
    for (int q = 0; q < 3; q++) {
        TaskInfo *t = queues[q]->head;
        while (t) {
            TaskInfo *next = t->next;
            
            /* Varış zamanından itibaren 20 saniyeden fazla geçti mi? */
            if ((currentTime - t->arrivalTime) > TIMEOUT_SECONDS) {
                printTimeout(t, currentTime);
                t->state = STATE_TIMEOUT;
                removeFromQueue(queues[q], t);
                
                if (t->handle) {
                    vTaskDelete(t->handle);
                    t->handle = NULL;
                }
            }
            t = next;
        }
    }
}

/*****************************************************************************
 * SCHEDULER BAŞLATMA
 *****************************************************************************/

void SchedulerInit(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Dosya açılamadı");
        exit(1);
    }
    
    char line[256];
    int arrival, priority, burst;
    
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '#') continue;
        
        if (sscanf(line, "%d, %d, %d", &arrival, &priority, &burst) == 3) {
            TaskInfo *t = (TaskInfo *)malloc(sizeof(TaskInfo));
            t->id = totalTasks;
            t->arrivalTime = arrival;
            t->originalPriority = priority;
            t->currentPriority = priority;
            t->burstTime = burst;
            t->remainingTime = burst;
            t->state = STATE_PENDING;
            t->handle = NULL;
            t->color = colors[totalTasks % NUM_COLORS];
            t->next = NULL;
            
            /* Worker task oluştur */
            char name[16];
            snprintf(name, sizeof(name), "T%d", totalTasks);
            xTaskCreate(WorkerTask, name, configMINIMAL_STACK_SIZE, t, 1, &t->handle);
            if (t->handle) vTaskSuspend(t->handle);
            
            allTasks[totalTasks++] = t;
        }
    }
    fclose(file);
    
    /* Scheduler task */
    xTaskCreate(SchedulerTask, "Sched", configMINIMAL_STACK_SIZE * 4, NULL,
                configMAX_PRIORITIES - 1, NULL);
}

/*****************************************************************************
 * ANA ZAMANLAYICI
 *****************************************************************************/

void SchedulerTask(void *pvParameters) {
    (void)pvParameters;
    
    TaskInfo *running = NULL;
    int justStarted = 0;  /* Bu tick'te yeni mi başladı? */
    
    for (;;) {
        /********************************************************************
         * 1. YENİ VARAN GÖREVLERİ KUYRUĞA EKLE
         ********************************************************************/
        for (int i = 0; i < totalTasks; i++) {
            TaskInfo *t = allTasks[i];
            if (t && t->state == STATE_PENDING && t->arrivalTime == globalTime) {
                t->state = STATE_READY;
                Queue *q = getQueue(t->currentPriority);
                enqueue(q, t);
            }
        }
        
        /********************************************************************
         * 2. RT PREEMPTION KONTROLÜ
         * Eğer kullanıcı görevi çalışıyorsa ve RT kuyruğunda görev varsa
         ********************************************************************/
        if (running && running->state == STATE_RUNNING && 
            running->currentPriority > 0 && rtQueue.count > 0) {
            
            /* Kullanıcı görevini askıya al */
            running->currentPriority++;  /* Öncelik düşür */
            running->state = STATE_SUSPENDED;
            printSuspended(running, globalTime);
            
            /* Uygun kuyruğa ekle */
            Queue *q = getQueue(running->currentPriority);
            enqueue(q, running);
            
            running = NULL;
        }
        
        /********************************************************************
         * 3. ÇALIŞTIRILACAK GÖREVİ SEÇ
         ********************************************************************/
        if (!running) {
            TaskInfo *next = peekHighestPriority();
            
            if (next) {
                Queue *q = getQueue(next->currentPriority);
                dequeue(q);
                
                running = next;
                running->state = STATE_RUNNING;
                justStarted = 1;
                
                /* Başladı mesajı */
                printStarted(running, globalTime);
            }
        }
        
        /********************************************************************
         * 4. GÖREVI ÇALIŞTIR (1 KUANTUM)
         ********************************************************************/
        if (running && running->state == STATE_RUNNING) {
            
            /* Eğer bu tick'te yeni başlamadıysa, yürütülüyor yazdır */
            if (!justStarted) {
                printRunning(running, globalTime);
            }
            justStarted = 0;
            
            /* 1 birim çalıştır */
            running->remainingTime--;
            
            /* Zaman ilerle (mesajlar için) */
            globalTime++;
            
            /****************************************************************
             * 5. GÖREV BİTTİ Mİ?
             ****************************************************************/
            if (running->remainingTime == 0) {
                printFinished(running, globalTime);
                running->state = STATE_FINISHED;
                
                if (running->handle) {
                    vTaskDelete(running->handle);
                    running->handle = NULL;
                }
                running = NULL;
                
                /* Timeout kontrolü */
                checkTimeouts(globalTime);
                
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            
            /****************************************************************
             * 6. RT GÖREVİ İSE KESİNTİSİZ DEVAM ET
             ****************************************************************/
            if (running->currentPriority == 0) {
                /* RT görevi, kesintisiz devam */
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            
            /****************************************************************
             * 7. KULLANICI GÖREVİ - QUANTUM BİTTİ
             * Feedback ve Round-Robin mantığı
             ****************************************************************/
            
            /* Daha yüksek öncelikli görev var mı? */
            int shouldPreempt = 0;
            
            if (running->currentPriority >= 1 && rtQueue.count > 0) shouldPreempt = 1;
            if (running->currentPriority >= 2 && fbQueue1.count > 0) shouldPreempt = 1;
            if (running->currentPriority >= 3 && fbQueue2.count > 0) shouldPreempt = 1;
            
            /* Aynı seviyede başka görev var mı? (Round-Robin) */
            Queue *myQueue = getQueue(running->currentPriority);
            if (myQueue->count > 0) shouldPreempt = 1;
            
            if (shouldPreempt) {
                /* Önceliği düşür */
                running->currentPriority++;
                running->state = STATE_SUSPENDED;
                
                printSuspended(running, globalTime);
                
                /* Yeni kuyruğa ekle */
                Queue *newQ = getQueue(running->currentPriority);
                enqueue(newQ, running);
                
                running = NULL;
            }
            /* else: Tek başına, devam edecek */
            
            /* Timeout kontrolü */
            checkTimeouts(globalTime);
        }
        else {
            /* Çalışan görev yok */
            globalTime++;
            checkTimeouts(globalTime);
        }
        
        /********************************************************************
         * 8. SİMÜLASYON BİTTİ Mİ?
         ********************************************************************/
        if (!running && allQueuesEmpty() && !hasPendingTasks()) {
            printf("\nSimülasyon tamamlandı. Toplam süre: %d sn\n", globalTime);
            vTaskDelay(pdMS_TO_TICKS(100));
            exit(0);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}