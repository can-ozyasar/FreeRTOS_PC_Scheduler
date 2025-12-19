/* src/scheduler.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "scheduler.h"

#define MAX_TASKS 100
#define TIMEOUT_LIMIT 20

Queue qRT = {NULL, NULL, 0};
Queue qP1 = {NULL, NULL, 0};
Queue qP2 = {NULL, NULL, 0};
Queue qP3 = {NULL, NULL, 0};

TaskInfo *pendingTasks[MAX_TASKS];
int pendingCount = 0;
int globalTime = 0;

const char *colors[] = {COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN};

/* -------------------- KUYRUK FONKSİYONLARI -------------------- */

// Kuyruğun sonuna ekle
void enqueue(Queue* q, TaskInfo* t) {
    t->next = NULL;
    if (q->tail != NULL)
        q->tail->next = t;
    q->tail = t;
    if (q->head == NULL)
        q->head = t;
    q->count++;
}

// Kuyruğun başından çıkar
TaskInfo* dequeue(Queue* q) {
    if (q->head == NULL) return NULL;
    TaskInfo* temp = q->head;
    q->head = q->head->next;
    if (q->head == NULL)
        q->tail = NULL;
    q->count--;
    temp->next = NULL;
    return temp;
}

// Belirli bir elemanı kuyruktan çıkar
void remove_from_queue(Queue* q, TaskInfo* t) {
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
        if (q->tail == t)
            q->tail = current;
        q->count--;
        t->next = NULL;
    }
}

// Önceliğe göre uygun kuyruğu döndür
Queue* get_queue_for_priority(int priority) {
    if (priority == 0) return &qRT;
    if (priority == 1) return &qP1;
    if (priority == 2) return &qP2;
    return &qP3;  // 3 ve üzeri hep qP3'e
}

// Prosesi uygun kuyruğa ekle
void add_to_appropriate_queue(TaskInfo* t) {
    Queue* q = get_queue_for_priority(t->priority);
    enqueue(q, t);
}

// Prosesi mevcut kuyruğundan çıkar (önceliğini bilmeden)
void remove_from_current_queue(TaskInfo* t) {
    // Tüm kuyruklarda ara ve çıkar
    remove_from_queue(&qRT, t);
    remove_from_queue(&qP1, t);
    remove_from_queue(&qP2, t);
    remove_from_queue(&qP3, t);
}

/* -------------------- YARDIMCI FONKSİYONLAR -------------------- */

// En yüksek öncelikli (en düşük değer) bekleyen prosesi bul
TaskInfo* get_highest_priority_task() {
    if (qRT.head != NULL) return qRT.head;
    if (qP1.head != NULL) return qP1.head;
    if (qP2.head != NULL) return qP2.head;
    if (qP3.head != NULL) return qP3.head;
    return NULL;
}

// Tüm kuyruklar boş mu?
int all_queues_empty() {
    return (qRT.head == NULL && qP1.head == NULL && 
            qP2.head == NULL && qP3.head == NULL);
}

// Bekleyen görev var mı?
int has_pending_tasks() {
    for (int i = 0; i < pendingCount; i++) {
        if (pendingTasks[i] != NULL)
            return 1;
    }
    return 0;
}

/* -------------------- ZAMANAŞIMI KONTROLÜ -------------------- */

// Timeout kontrolü: Varış zamanından itibaren 20 saniye geçen görevleri sonlandır
void CheckTimeouts() {
    Queue* queues[] = {&qP1, &qP2, &qP3};  // Sadece kullanıcı kuyrukları
    
    for (int i = 0; i < 3; i++) {
        Queue* q = queues[i];
        TaskInfo* curr = q->head;
        
        while (curr != NULL) {
            TaskInfo* nextNode = curr->next;
            
            // Varış zamanından itibaren 20 saniye geçti mi?
            int waiting_time = globalTime - curr->arrivalTime;
            
            if (waiting_time >= TIMEOUT_LIMIT && curr->remainingTime > 0) {
                printf("%s%.4f sn\tproses zamanaşımı\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       curr->color, (double)globalTime, curr->id, 
                       curr->priority, curr->remainingTime, COLOR_RESET);
                
                curr->isFinished = 1;
                remove_from_queue(q, curr);
                
                if (curr->handle != NULL)
                    vTaskDelete(curr->handle);
            }
            
            curr = nextNode;
        }
    }
}

/* -------------------- ANA ZAMANLAYICI -------------------- */

void SchedulerInit(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Dosya hatasi");
        exit(1);
    }
    
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
            t->isFinished = 0;
            t->hasStarted = 0;
            t->wasRunningBefore = 0;
            t->next = NULL;
            t->color = colors[colorIdx % 6];
            colorIdx++;
            
            xTaskCreate(WorkerTask, "Task", configMINIMAL_STACK_SIZE, t, 1, &t->handle);
            vTaskSuspend(t->handle);
            
            pendingTasks[pendingCount++] = t;
        }
    }
    
    fclose(file);
    
    xTaskCreate(SchedulerTask, "Scheduler", configMINIMAL_STACK_SIZE * 2, NULL, 
                configMAX_PRIORITIES - 1, NULL);
}

void SchedulerTask(void *pvParameters) {
    TaskInfo *currentTask = NULL;
    
    for (;;) {
        /* ========== 1. YENİ GELEN GÖREVLERİ KUYRUĞA EKLE ========== */
        for (int i = 0; i < pendingCount; i++) {
            if (pendingTasks[i] != NULL && pendingTasks[i]->arrivalTime == globalTime) {
                TaskInfo* t = pendingTasks[i];
                add_to_appropriate_queue(t);
                pendingTasks[i] = NULL;
            }
        }
        
        /* ========== 2. PREEMPTION KONTROLÜ ========== */
        // Eğer çalışan bir görev varsa ve daha yüksek öncelikli görev geldiyse
        TaskInfo* nextTask = get_highest_priority_task();
        
        if (currentTask != NULL && currentTask->remainingTime > 0 && nextTask != NULL) {
            // RT görevi çalışıyorsa preemption olmaz (sadece başka RT preempt edemez)
            // Ama yeni RT gelirse mevcut RT devam eder (FCFS)
            
            if (currentTask->priority > 0) {
                // Kullanıcı görevi çalışıyor
                // Daha yüksek öncelikli (daha düşük değer) görev var mı?
                if (nextTask->priority < currentTask->priority) {
                    // PREEMPTION! Mevcut görevi askıya al
                    
                    // Önceliği artır (değer artar = öncelik düşer)
                    currentTask->priority++;
                    
                    // Askıya alındı mesajı
                    printf("%s%.4f sn\tproses askıda\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                           currentTask->color, (double)globalTime, currentTask->id,
                           currentTask->priority, currentTask->remainingTime, COLOR_RESET);
                    
                    // Mevcut kuyruktan çıkar ve yeni kuyruğa ekle
                    remove_from_current_queue(currentTask);
                    add_to_appropriate_queue(currentTask);
                    
                    currentTask->wasRunningBefore = 1;
                    currentTask = NULL;
                }
            }
        }
        
        /* ========== 3. ÇALIŞTIRILACAK GÖREVİ SEÇ ========== */
        nextTask = get_highest_priority_task();
        
        if (nextTask == NULL) {
            // Çalışacak görev yok, simülasyon bitti mi kontrol et
            if (!has_pending_tasks() && all_queues_empty()) {
                vTaskDelay(pdMS_TO_TICKS(100));
                exit(0);
            }
            
            globalTime++;
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        /* ========== 4. GÖREVİ ÇALIŞTIR ========== */
        currentTask = nextTask;
        
        // Başladı veya Yürütülüyor mesajı
        if (currentTask->hasStarted == 0) {
            // İlk kez başlıyor
            printf("%s%.4f sn\tproses başladı\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                   currentTask->color, (double)globalTime, currentTask->id,
                   currentTask->priority, currentTask->remainingTime, COLOR_RESET);
            currentTask->hasStarted = 1;
        } else if (currentTask->wasRunningBefore) {
            // Askıdan dönüyor - "başladı" yazdır
            printf("%s%.4f sn\tproses başladı\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                   currentTask->color, (double)globalTime, currentTask->id,
                   currentTask->priority, currentTask->remainingTime, COLOR_RESET);
            currentTask->wasRunningBefore = 0;
        } else {
            // Devam ediyor
            printf("%s%.4f sn\tproses yürütülüyor\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                   currentTask->color, (double)globalTime, currentTask->id,
                   currentTask->priority, currentTask->remainingTime, COLOR_RESET);
        }
        
        // 1 birim zaman çalıştır
        currentTask->remainingTime--;
        
        /* ========== 5. GÖREV BİTTİ Mİ? ========== */
        if (currentTask->remainingTime == 0) {
            // Görev tamamlandı
            printf("%s%.4f sn\tproses sonlandı\t\t(id:%04d\töncelik:%d\tkalan süre:0 sn)%s\n",
                   currentTask->color, (double)(globalTime + 1), currentTask->id,
                   currentTask->priority, COLOR_RESET);
            
            currentTask->isFinished = 1;
            remove_from_current_queue(currentTask);
            
            if (currentTask->handle != NULL)
                vTaskDelete(currentTask->handle);
            
            currentTask = NULL;
        }
        else if (currentTask->priority > 0) {
            // Kullanıcı görevi, 1 kuantum çalıştı, askıya al ve öncelik düşür
            
            // Kuyrukta başka görev var mı kontrol et (kendisi hariç)
            TaskInfo* otherTask = get_highest_priority_task();
            
            // Eğer kuyrukta sadece kendisi varsa veya kendisi en yüksek öncelikliyse
            // ve başka aynı/daha yüksek öncelikli görev yoksa devam edebilir
            
            // Round-robin için: Aynı öncelik kuyruğunda başka görev var mı?
            Queue* currentQueue = get_queue_for_priority(currentTask->priority);
            
            int shouldPreempt = 0;
            
            // Daha yüksek öncelikli kuyrukta görev var mı?
            if (currentTask->priority >= 1 && qRT.head != NULL) shouldPreempt = 1;
            if (currentTask->priority >= 2 && qP1.head != NULL && qP1.head != currentTask) shouldPreempt = 1;
            if (currentTask->priority >= 3 && qP2.head != NULL && qP2.head != currentTask) shouldPreempt = 1;
            
            // Aynı kuyrukta birden fazla görev varsa round-robin
            if (currentQueue->count > 1) shouldPreempt = 1;
            
            if (shouldPreempt) {
                // Önceliği artır
                currentTask->priority++;
                
                // Askıya alındı mesajı (bir sonraki saniyenin başında)
                printf("%s%.4f sn\tproses askıda\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       currentTask->color, (double)(globalTime + 1), currentTask->id,
                       currentTask->priority, currentTask->remainingTime, COLOR_RESET);
                
                // Kuyruktan çıkar ve yeni kuyruğa ekle
                remove_from_current_queue(currentTask);
                add_to_appropriate_queue(currentTask);
                
                currentTask->wasRunningBefore = 1;
                currentTask = NULL;
            }
            // else: Tek başına, devam eder
        }
        // RT görevi (priority == 0): Kesintisiz devam eder
        
        /* ========== 6. ZAMANI İLERLET VE TIMEOUT KONTROLÜ ========== */
        globalTime++;
        CheckTimeouts();
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}