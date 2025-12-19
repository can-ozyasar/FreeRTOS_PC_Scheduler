/* src/scheduler.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"
#include "tasks.h"  // ÖNEMLİ: WorkerTask'ı görmek için gerekli

#define MAX_TASKS       100
#define TIMEOUT_LIMIT   20
#define QUANTUM         1

// Global Değişkenler
Queue qRT = {NULL, NULL, 0};
Queue qP1 = {NULL, NULL, 0};
Queue qP2 = {NULL, NULL, 0};
Queue qP3 = {NULL, NULL, 0};

TaskInfo* pendingTasks[MAX_TASKS];
int pendingCount = 0;
int globalTime = 0;
TaskInfo* runningTask = NULL;

const char* colors[] = {
    COLOR_RED, COLOR_GREEN, COLOR_YELLOW, 
    COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN
};

// Kuyruk Fonksiyonları
void enqueue(Queue* q, TaskInfo* t) {
    t->next = NULL;
    if (q->tail != NULL) q->tail->next = t;
    q->tail = t;
    if (q->head == NULL) q->head = t;
    q->count++;
}

TaskInfo* dequeue(Queue* q) {
    if (q->head == NULL) return NULL;
    TaskInfo* temp = q->head;
    q->head = q->head->next;
    if (q->head == NULL) q->tail = NULL;
    temp->next = NULL;
    q->count--;
    return temp;
}

void removeFromQueue(Queue* q, TaskInfo* t) {
    if (q->head == NULL || t == NULL) return;
    if (q->head == t) {
        dequeue(q);
        return;
    }
    TaskInfo* current = q->head;
    while (current->next != NULL && current->next != t) {
        current = current->next;
    }
    if (current->next == t) {
        current->next = t->next;
        if (q->tail == t) q->tail = current;
        q->count--;
        t->next = NULL;
    }
}

Queue* getQueueByPriority(int priority) {
    switch (priority) {
        case PRIORITY_RT:   return &qRT;
        case PRIORITY_HIGH: return &qP1;
        case PRIORITY_MED:  return &qP2;
        default:            return &qP3;
    }
}

void checkTimeouts(void) {
    Queue* userQueues[] = {&qP1, &qP2, &qP3};
    for (int i = 0; i < 3; i++) {
        Queue* q = userQueues[i];
        TaskInfo* curr = q->head;
        while (curr != NULL) {
            TaskInfo* nextNode = curr->next;
            curr->waitCounter++;
            if (curr->waitCounter >= TIMEOUT_LIMIT) {
                printf("%s%d.0000 sn\tproses zamanaşımı\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       curr->color, globalTime, curr->id, 
                       curr->priority, curr->remainingTime, COLOR_RESET);
                curr->isFinished = 1;
                removeFromQueue(q, curr);
                if (curr->handle) vTaskDelete(curr->handle);
            }
            curr = nextNode;
        }
    }
}

int isSimulationComplete(void) {
    for (int i = 0; i < pendingCount; i++) if (pendingTasks[i] != NULL) return 0;
    if (qRT.head || qP1.head || qP2.head || qP3.head) return 0;
    if (runningTask != NULL) return 0;
    return 1;
}

void SchedulerInit(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) { perror("Dosya açılamadı"); exit(1); }
    
    char line[256];
    int arrival, priority, burst, colorIdx = 0;
    
    printf("=== GÖREV LİSTESİ YÜKLENİYOR ===\n");
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '\r') continue;
        if (sscanf(line, "%d, %d, %d", &arrival, &priority, &burst) == 3) {
            TaskInfo* t = (TaskInfo*)malloc(sizeof(TaskInfo));
            t->id = pendingCount;
            t->arrivalTime = arrival;
            t->originalPriority = priority;
            t->priority = priority;
            t->burstTime = burst;
            t->remainingTime = burst;
            t->waitCounter = 0;
            t->isFinished = 0;
            t->hasStarted = 0;
            t->next = NULL;
            t->color = colors[colorIdx % 6];
            colorIdx++;
            
            xTaskCreate(WorkerTask, "Worker", configMINIMAL_STACK_SIZE, t, 1, &t->handle);
            vTaskSuspend(t->handle);
            pendingTasks[pendingCount++] = t;
            printf("  Görev %04d: Varış=%d, Öncelik=%d, Süre=%d\n", t->id, arrival, priority, burst);
        }
    }
    fclose(file);
    printf("=== TOPLAM %d GÖREV YÜKLENDİ ===\n\n", pendingCount);
    xTaskCreate(SchedulerTask, "Scheduler", configMINIMAL_STACK_SIZE * 4, NULL, configMAX_PRIORITIES - 1, NULL);
}

void SchedulerTask(void* pvParameters) {
    (void)pvParameters;
    printf("=== ZAMANLAYICI BAŞLADI ===\n\n");
    
    for (;;) {
        // 1. Yeni gelenleri ekle
        for (int i = 0; i < pendingCount; i++) {
            if (pendingTasks[i] != NULL && pendingTasks[i]->arrivalTime == globalTime) {
                enqueue(getQueueByPriority(pendingTasks[i]->priority), pendingTasks[i]);
                pendingTasks[i] = NULL;
            }
        }
        
        // 2. Preemption (RT Kesmesi)
        if (qRT.head != NULL && runningTask != NULL && runningTask->priority != PRIORITY_RT) {
            if (runningTask->priority < MAX_PRIORITY) runningTask->priority++;
            printf("%s%d.0000 sn\tproses askıda\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                   runningTask->color, globalTime, runningTask->id,
                   runningTask->priority, runningTask->remainingTime, COLOR_RESET);
            enqueue(getQueueByPriority(runningTask->priority), runningTask);
            runningTask = NULL;
        }
        
        // 3. Görev Seç
        if (runningTask == NULL) {
            if (qRT.head) runningTask = dequeue(&qRT);
            else if (qP1.head) runningTask = dequeue(&qP1);
            else if (qP2.head) runningTask = dequeue(&qP2);
            else if (qP3.head) runningTask = dequeue(&qP3);
        }
        
        // 4. Çalıştır
        if (runningTask != NULL) {
            runningTask->waitCounter = 0;
            if (!runningTask->hasStarted) {
                printf("%s%d.0000 sn\tproses başladı\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       runningTask->color, globalTime, runningTask->id, runningTask->priority, runningTask->remainingTime, COLOR_RESET);
                runningTask->hasStarted = 1;
            } else {
                printf("%s%d.0000 sn\tproses yürütülüyor\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       runningTask->color, globalTime, runningTask->id, runningTask->priority, runningTask->remainingTime, COLOR_RESET);
            }
            
            runningTask->remainingTime--;
            
            // 5. Durum Değerlendirme
            if (runningTask->remainingTime == 0) {
                printf("%s%d.0000 sn\tproses sonlandı\t\t(id:%04d\töncelik:%d\tkalan süre:0 sn)%s\n",
                       runningTask->color, globalTime + 1, runningTask->id, runningTask->priority, COLOR_RESET);
                runningTask->isFinished = 1;
                if (runningTask->handle) vTaskDelete(runningTask->handle);
                runningTask = NULL;
            } else if (runningTask->priority != PRIORITY_RT) {
                if (runningTask->priority < MAX_PRIORITY) runningTask->priority++;
                printf("%s%d.0000 sn\tproses askıda\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       runningTask->color, globalTime + 1, runningTask->id, runningTask->priority, runningTask->remainingTime, COLOR_RESET);
                enqueue(getQueueByPriority(runningTask->priority), runningTask);
                runningTask = NULL;
            }
        }
        
        globalTime++;
        checkTimeouts();
        
        if (isSimulationComplete() && globalTime > 0) {
            printf("\n=== SİMÜLASYON SONA ERDİ (Toplam süre: %d saniye) ===\n", globalTime);
            vTaskDelay(pdMS_TO_TICKS(100));
            exit(0);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}