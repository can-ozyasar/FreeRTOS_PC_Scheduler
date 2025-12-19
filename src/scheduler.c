/* src/scheduler.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "scheduler.h"

#define MAX_TASKS 100
#define TIMEOUT_LIMIT 20 // 20 sn kuralı

Queue qRT = {NULL, NULL, 0};
Queue qP1 = {NULL, NULL, 0};
Queue qP2 = {NULL, NULL, 0};
Queue qP3 = {NULL, NULL, 0}; // P3 ve daha düşük önceliklerin hepsi burada (P3, P4, P5...)

TaskInfo* pendingTasks[MAX_TASKS];
int pendingCount = 0;

int globalTime = 0;
const char* colors[] = {COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN};

// --- KUYRUK FONKSİYONLARI ---
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
    q->count--;
    return temp;
}

void remove_from_queue(Queue* q, TaskInfo* t) {
    if (q->head == NULL) return;
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
    }
}

// --- ZAMANAŞIMI KONTROLÜ ---
void CheckTimeouts() {
    // Sadece Kullanıcı Kuyruklarını kontrol et
    Queue* queues[] = {&qP1, &qP2, &qP3};
    
    for (int i = 0; i < 3; i++) {
        Queue* q = queues[i];
        TaskInfo* curr = q->head;
        TaskInfo* nextNode = NULL;

        while (curr != NULL) {
            nextNode = curr->next;
            
            // Bekleme süresini artır
            curr->waitCounter++;

            // Hoca çıktısında ID:0000 (0. sn geliş) -> 21. sn'de timeout oluyor.
            // Bu da tam 20 sn BEKLEME süresi demektir (waitCounter >= 20).
            if (curr->waitCounter >= TIMEOUT_LIMIT) {
                printf("%s%d.0000 sn\tproses zamanaşımı\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       curr->color, globalTime, curr->id, curr->priority, curr->remainingTime, COLOR_RESET);
                
                curr->isFinished = 1;
                remove_from_queue(q, curr);
                vTaskDelete(curr->handle);
            }
            curr = nextNode;
        }
    }
}

void SchedulerInit(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) { perror("Dosya hatasi"); exit(1); }

    char line[256];
    int arrival, priority, burst;
    int colorIdx = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%d, %d, %d", &arrival, &priority, &burst) == 3) {
            TaskInfo* t = (TaskInfo*)malloc(sizeof(TaskInfo));
            t->id = pendingCount; 
            t->arrivalTime = arrival;
            t->priority = priority;
            t->burstTime = burst;
            t->remainingTime = burst;
            t->waitCounter = 0;
            t->isFinished = 0;
            t->hasStarted = 0;
            t->next = NULL;
            t->color = colors[colorIdx % 6];
            colorIdx++;

            xTaskCreate(WorkerTask, "Task", configMINIMAL_STACK_SIZE, t, 1, &t->handle);
            vTaskSuspend(t->handle); 
            pendingTasks[pendingCount++] = t;
        }
    }
    fclose(file);
    xTaskCreate(SchedulerTask, "Scheduler", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 1, NULL);
}

void SchedulerTask(void *pvParameters) {
    TaskInfo* currentTask = NULL;
    
    for (;;) {
        // 1. YENİ GELENLERİ AL
        for (int i = 0; i < pendingCount; i++) {
            if (pendingTasks[i] != NULL && pendingTasks[i]->arrivalTime == globalTime) {
                TaskInfo* t = pendingTasks[i];
                if (t->priority == 0) enqueue(&qRT, t);
                else if (t->priority == 1) enqueue(&qP1, t);
                else if (t->priority == 2) enqueue(&qP2, t);
                else enqueue(&qP3, t); // P3 ve üzeri
                pendingTasks[i] = NULL;
            }
        }

        // 2. GÖREV SEÇİMİ
        TaskInfo* nextTask = NULL;

        if (qRT.head != NULL) {
            nextTask = qRT.head;
        } else {
            if (qP1.head != NULL) nextTask = qP1.head;
            else if (qP2.head != NULL) nextTask = qP2.head;
            else if (qP3.head != NULL) nextTask = qP3.head;
        }

        // 3. YÜRÜTME
        if (nextTask != NULL) {
            currentTask = nextTask;
            currentTask->waitCounter = 0; // Çalıştığı için bekleme sıfırlanır

            // Başladı / Yürütülüyor Mesajı
            if (currentTask->hasStarted == 0) {
                printf("%s%d.0000 sn\tproses başladı\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       currentTask->color, globalTime, currentTask->id, currentTask->priority, currentTask->remainingTime, COLOR_RESET);
                currentTask->hasStarted = 1;
            } else {
                printf("%s%d.0000 sn\tproses yürütülüyor\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       currentTask->color, globalTime, currentTask->id, currentTask->priority, currentTask->remainingTime, COLOR_RESET);
            }

            currentTask->remainingTime--;

            // GÖREV BİTTİ Mİ?
            if (currentTask->remainingTime == 0) {
                printf("%s%d.0000 sn\tproses sonlandı\t\t(id:%04d\töncelik:%d\tkalan süre:0 sn)%s\n",
                       currentTask->color, globalTime + 1, currentTask->id, currentTask->priority, COLOR_RESET);
                
                currentTask->isFinished = 1;
                // Kuyruktan çıkar
                if (currentTask->priority == 0) dequeue(&qRT);
                else if (currentTask->priority == 1) dequeue(&qP1);
                else if (currentTask->priority == 2) dequeue(&qP2);
                else dequeue(&qP3); // P3, P4, P5... hepsi buradan çıkar
                
            } else {
                // BİTMEDİ - ÖNCELİK YÖNETİMİ
                if (currentTask->priority == 0) {
                    // RT görevi: Kuyruğun başında kalır, devam eder.
                } else {
                    // Kullanıcı görevi: Öncelik HER ZAMAN artar (değer düşer)
                    
                    // Önce mevcut kuyruktan çıkar
                    if (currentTask->priority == 1) dequeue(&qP1);
                    else if (currentTask->priority == 2) dequeue(&qP2);
                    else dequeue(&qP3); // P3 ve üstü buradan çıkar

                    // !!! KRİTİK DÜZELTME !!!
                    // Öncelik 3'te kilitlenmez, sonsuza kadar artar (3->4->5->6...)
                    currentTask->priority++;
                    
                    // Askıya alındı mesajı (Bir sonraki saniyenin başında)
                    printf("%s%d.0000 sn\tproses askıda\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                           currentTask->color, globalTime + 1, currentTask->id, currentTask->priority, currentTask->remainingTime, COLOR_RESET);

                    // Yeni kuyruğa ekle
                    // 1 ise 2'ye, 2 ise 3'e, 3 ve üzeri ise yine P3 kuyruğuna (en sonuna)
                    if (currentTask->priority == 1) enqueue(&qP1, currentTask); // Teorik
                    else if (currentTask->priority == 2) enqueue(&qP2, currentTask);
                    else enqueue(&qP3, currentTask); // Öncelik 4, 5 olsa bile P3 kuyruğuna girer
                }
            }
        }

        // 4. ZAMAN İLERLET VE TIMEOUT KONTROLÜ
        // Önce zamanı artırıyoruz, sonra timeout kontrolü yapıyoruz.
        // Böylece 0'da gelen task, 20. saniyenin SONUNDA (21. saniyenin başında) timeout oluyor.
        globalTime++;
        CheckTimeouts();

        // SİMÜLASYON BİTİŞİ
        int allDone = 1;
        for(int k=0; k<MAX_TASKS; k++) if(pendingTasks[k] != NULL) allDone = 0;
        if (qRT.head || qP1.head || qP2.head || qP3.head) allDone = 0;
        
        if (allDone && globalTime > 1) {
             vTaskDelay(pdMS_TO_TICKS(100)); 
             exit(0);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
