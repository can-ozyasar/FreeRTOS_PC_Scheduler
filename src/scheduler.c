/* src/scheduler.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

#define MAX_TASKS 100
#define TIMEOUT_LIMIT 20

// Kuyruklar: RT(0), P1, P2, P3
Queue qRT = {NULL, NULL, 0};
Queue qP1 = {NULL, NULL, 0};
Queue qP2 = {NULL, NULL, 0};
Queue qP3 = {NULL, NULL, 0};

TaskInfo *pendingTasks[MAX_TASKS]; // Henüz gelmemiş görevler
int pendingCount = 0;
int globalTime = 0;

// Renk havuzu
const char *colors[] = {COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN};

/* -------------------- KUYRUK YÖNETİMİ -------------------- */

void enqueue(Queue* q, TaskInfo* t) {
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

TaskInfo* dequeue(Queue* q) {
    if (q->head == NULL) return NULL;
    TaskInfo* temp = q->head;
    q->head = q->head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    q->count--;
    temp->next = NULL;
    return temp;
}

// Belirli bir görevi kuyruktan (aradan veya baştan) silmek için
void remove_task_from_queue(Queue* q, TaskInfo* t) {
    if (q->head == NULL) return;

    if (q->head == t) {
        dequeue(q);
        return;
    }

    TaskInfo* prev = q->head;
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

/* -------------------- YARDIMCI FONKSİYONLAR -------------------- */

// Öncelik değerine göre (0,1,2,3) doğru kuyruğu döndürür
Queue* get_queue_by_priority(int p) {
    switch(p) {
        case 0: return &qRT;
        case 1: return &qP1;
        case 2: return &qP2;
        default: return &qP3;
    }
}

// En yüksek öncelikli kuyruğun başındaki görevi seç (Kuyruktan çıkarmaz, sadece bakar)
TaskInfo* pick_next_task() {
    if (qRT.head != NULL) return qRT.head;
    if (qP1.head != NULL) return qP1.head;
    if (qP2.head != NULL) return qP2.head;
    if (qP3.head != NULL) return qP3.head;
    return NULL;
}

int has_pending_tasks() {
    for(int i=0; i<pendingCount; i++) {
        if(pendingTasks[i] != NULL) return 1;
    }
    return 0;
}

int all_queues_empty() {
    return (qRT.count == 0 && qP1.count == 0 && qP2.count == 0 && qP3.count == 0);
}

/* -------------------- DOSYA OKUMA VE INIT -------------------- */

void SchedulerInit(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Hata: Dosya acilamadi!\n");
        exit(1);
    }

    char line[256];
    int arrival, priority, burst;
    int colorIdx = 0;

    // Dosyadan okuma formatı: arrival, priority, burst
    while (fgets(line, sizeof(line), file)) {
        // Satırdaki boşlukları tolere etmek için format string
        if (sscanf(line, "%d, %d, %d", &arrival, &priority, &burst) == 3) {
            TaskInfo* t = (TaskInfo*)malloc(sizeof(TaskInfo));
            t->id = pendingCount; // ID 0'dan başlar
            t->arrivalTime = arrival;
            t->priority = priority;
            t->burstTime = burst;
            t->remainingTime = burst;
            t->isFinished = 0;
            t->hasStarted = 0;
            t->next = NULL;
            t->color = colors[colorIdx % 6];
            colorIdx++;

            // WorkerTask oluştur ama hemen çalıştırma (Scheduler yönetecek)
            xTaskCreate(WorkerTask, "Worker", configMINIMAL_STACK_SIZE, t, 1, &t->handle);
            // vTaskSuspend(t->handle); // İsteğe bağlı, zaten WorkerTask delay ile bekliyor

            pendingTasks[pendingCount++] = t;
        }
    }
    fclose(file);

    // Scheduler görevini oluştur
    xTaskCreate(SchedulerTask, "Scheduler", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 1, NULL);
}

/* -------------------- ZAMAN AŞIMI KONTROLÜ -------------------- */
void check_timeouts() {
    Queue* queues[] = {&qP1, &qP2, &qP3}; // Sadece kullanıcı kuyruklarında timeout olur mu? Raporda genel konuşmuş ama genelde RT timeout yemez. Biz hepsine bakalım güvenli olsun diye ancak genelde User Queue bakılır.
    // Rapora göre: "Görevlendirici tarafından sonlandırılmazsa, görev 20 saniye sonra kendiliğinden sona erecektir."
    // Bu yüzden tüm kuyruklara bakmak en doğrusu.

    Queue* allQueues[] = {&qRT, &qP1, &qP2, &qP3};

    for(int i=0; i<4; i++) {
        Queue* q = allQueues[i];
        TaskInfo* curr = q->head;
        TaskInfo* nextNode;

        while(curr != NULL) {
            nextNode = curr->next; // Silinme ihtimaline karşı yedeği al
            
            // Varış zamanından itibaren 20 sn geçti mi?
            if ((globalTime - curr->arrivalTime) >= TIMEOUT_LIMIT) {
                printf("%s%d.0000 sn\tproses zamanaşımı\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       curr->color, globalTime, curr->id, curr->priority, curr->remainingTime, COLOR_RESET);
                
                curr->isFinished = 1;
                remove_task_from_queue(q, curr);
                vTaskDelete(curr->handle);
                free(curr);
            }
            curr = nextNode;
        }
    }
}

/* -------------------- SCHEDULER TASK (ANA DÖNGÜ) -------------------- */

void SchedulerTask(void *pvParameters) {
    TaskInfo* runningTask = NULL;

    for (;;) {
        // 1. ADIM: Yeni Gelenleri Kuyruğa Al
        for (int i = 0; i < pendingCount; i++) {
            if (pendingTasks[i] != NULL && pendingTasks[i]->arrivalTime == globalTime) {
                Queue* targetQ = get_queue_by_priority(pendingTasks[i]->priority);
                enqueue(targetQ, pendingTasks[i]);
                pendingTasks[i] = NULL; // Listeden çıkar
            }
        }

        // 2. ADIM: Timeout Kontrolü
        check_timeouts();

        // 3. ADIM: Çalışacak Görevi Seç (Preemptive Kontrol)
        TaskInfo* nextTask = pick_next_task();

        // Eğer şu an çalışan bir RT görevi varsa ve bitmediyse, onu kesemeyiz (RT FCFS).
        // Ancak daha yüksek öncelikli bir RT geldiyse? Raporda RT'ler FCFS der, yani ilk gelen bitene kadar çalışır.
        // O yüzden RT çalışıyorsa değiştirmeyiz.
        if (runningTask != NULL && runningTask->priority == 0 && runningTask->remainingTime > 0) {
            // RT görevi çalışmaya devam eder
            nextTask = runningTask; 
        }

        // Eğer bir değişim varsa (Context Switch)
        if (runningTask != NULL && runningTask != nextTask && runningTask->remainingTime > 0) {
            // Eski görev askıya alındı
             printf("%s%d.0000 sn\tproses askıda\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       runningTask->color, globalTime, runningTask->id, runningTask->priority, runningTask->remainingTime, COLOR_RESET);
            
             // Eski görevi kuyruğun uygun yerine taşıyacağız (Feedback mantığı aşağıda 5. adımda yapılıyor, 
             // ancak burada preemption olduysa, round robin mantığı veya feedback mantığı devreye girmeliydi.
             // Biz basitlik adına simülasyonu 1 saniyelik bloklar halinde düşündüğümüz için,
             // görev değişikliğini sadece "seçilen nextTask farklıysa" anlıyoruz.)
        }

        // Çalışacak görev yok mu?
        if (nextTask == NULL) {
            if (!has_pending_tasks() && all_queues_empty()) {
                printf("Tum gorevler tamamlandi. Simulasyon bitti.\n");
                exit(0);
            }
            globalTime++;
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // 4. ADIM: Görevi Yürüt
        runningTask = nextTask;
        
        // Başlangıç/Devam mesajı
        if (runningTask->hasStarted == 0) {
             printf("%s%d.0000 sn\tproses başladı\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       runningTask->color, globalTime, runningTask->id, runningTask->priority, runningTask->remainingTime, COLOR_RESET);
            runningTask->hasStarted = 1;
        } else {
             printf("%s%d.0000 sn\tproses yürütülüyor\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       runningTask->color, globalTime, runningTask->id, runningTask->priority, runningTask->remainingTime, COLOR_RESET);
        }

        // Simülasyon: 1 saniye bekle
        runningTask->remainingTime--;
        globalTime++;
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 Saniye gerçek zamanlı gecikme

        // 5. ADIM: Görev Durumu Güncelleme (Feedback & Termination)
        
        if (runningTask->remainingTime <= 0) {
            // Görev Bitti
             printf("%s%d.0000 sn\tproses sonlandı\t\t(id:%04d\töncelik:%d\tkalan süre:0 sn)%s\n",
                       runningTask->color, globalTime, runningTask->id, runningTask->priority, COLOR_RESET);
            
            runningTask->isFinished = 1;
            // Kuyruktan çıkar
            Queue* q = get_queue_by_priority(runningTask->priority);
            remove_task_from_queue(q, runningTask);
            vTaskDelete(runningTask->handle);
            free(runningTask);
            runningTask = NULL; // Görev yok edildi
        } 
        else {
            // Görev Bitmedi. Feedback Algoritması Uygula
            
            if (runningTask->priority == 0) {
                // RT Görevi: Öncelik değişmez, kuyruğun başında kalır (FCFS).
                // Bir şey yapmaya gerek yok.
            } 
            else {
                // User Görevi (1, 2, 3)
                // Mevcut kuyruktan çıkar
                Queue* oldQ = get_queue_by_priority(runningTask->priority);
                dequeue(oldQ); // Head'den çıkar (zaten runningTask head'deydi)

                // Önceliği Düşür (Değerini artır), Max 3
                if (runningTask->priority < 3) {
                    runningTask->priority++;
                }
                
                // Yeni (veya aynı) kuyruğun SONUNA ekle (Round Robin etkisi)
                Queue* newQ = get_queue_by_priority(runningTask->priority);
                enqueue(newQ, runningTask);
            }
        }
    }
}