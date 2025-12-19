/* src/scheduler.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "scheduler.h"

#define MAX_TASKS 100
#define TIMEOUT_LIMIT 20 // 20 saniye bekleme kuralı (20 sn bekledikten sonra 21. sn'de timeout)

// Dört Kuyruk: RT (Öncelik 0), P1 (Öncelik 1), P2 (Öncelik 2), P3+ (Öncelik 3 ve üzeri)
Queue qRT = {NULL, NULL, 0};
Queue qP1 = {NULL, NULL, 0};
Queue qP2 = {NULL, NULL, 0};
Queue qP3 = {NULL, NULL, 0};

TaskInfo* pendingTasks[MAX_TASKS];
int pendingCount = 0;

int globalTime = 0;
const char* colors[] = {COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN};

// ========== KUYRUK YÖNETİMİ FONKSİYONLARI ==========

/**
 * Kuyruğun sonuna görev ekler (FIFO mantığı)
 */
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

/**
 * Kuyruğun başından görev çıkarır
 */
TaskInfo* dequeue(Queue* q) {
    if (q->head == NULL) return NULL;
    TaskInfo* temp = q->head;
    q->head = q->head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    q->count--;
    return temp;
}

/**
 * Kuyruktan belirli bir görevi çıkarır (timeout için kullanılır)
 */
void remove_from_queue(Queue* q, TaskInfo* t) {
    if (q->head == NULL) return;
    
    // İlk eleman ise
    if (q->head == t) {
        dequeue(q);
        return;
    }
    
    // Ortada veya sonda ise
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
    }
}

// ========== ZAMANASIMI KONTROL FONKSİYONU ==========

/**
 * Kullanıcı kuyruklarındaki (P1, P2, P3) görevlerin bekleme sürelerini kontrol eder.
 * 20 saniye bekleyen görevler zamanaSIMI olur ve sonlandırılır.
 * 
 * NOT: RT kuyrukta (öncelik 0) zamanaSIMI OLMAZ - bu görevler öncelikli olarak çalışır.
 */
void CheckTimeouts() {
    Queue* queues[] = {&qP1, &qP2, &qP3};
    
    for (int i = 0; i < 3; i++) {
        Queue* q = queues[i];
        TaskInfo* curr = q->head;
        TaskInfo* nextNode = NULL;

        while (curr != NULL) {
            nextNode = curr->next;
            
            // Bekleme sayacını artır (bu görev bu saniye çalışmadı)
            curr->waitCounter++;

            // 20 saniye bekleme = 21. saniyede timeout
            // Örnek: 0. sn'de gelen -> 1,2,3...20 (20 kez artış) -> 21. sn'de timeout
            if (curr->waitCounter >= TIMEOUT_LIMIT) {
                printf("%s%d.0000 sn\tproses zamanaSIMI\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       curr->color, globalTime, curr->id, curr->priority, curr->remainingTime, COLOR_RESET);
                
                curr->isFinished = 1;
                remove_from_queue(q, curr);
                vTaskDelete(curr->handle);
            }
            
            curr = nextNode;
        }
    }
}

// ========== BAŞLANGIČ FONKSÄ°YONU ==========

/**
 * Görev listesini dosyadan okur ve görevleri oluşturur.
 * Her görev için FreeRTOS task oluşturulur ve askıya alınır.
 */
void SchedulerInit(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Dosya açma hatası");
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
            t->waitCounter = 0;
            t->isFinished = 0;
            t->hasStarted = 0;
            t->next = NULL;
            t->color = colors[colorIdx % 6];
            colorIdx++;

            // FreeRTOS görevi oluştur ve askıya al
            xTaskCreate(WorkerTask, "Task", configMINIMAL_STACK_SIZE, t, 1, &t->handle);
            vTaskSuspend(t->handle);
            
            pendingTasks[pendingCount++] = t;
        }
    }
    fclose(file);
    
    // Yüksek öncelikli Scheduler görevi oluştur
    xTaskCreate(SchedulerTask, "Scheduler", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 1, NULL);
}

// ========== ANA GÖREVLENDİRİCİ FONKSİYONU ==========

/**
 * Ana görevlendirici döngüsü. Her saniye:
 * 1. Yeni gelen görevleri kuyruklara yerleştirir
 * 2. En yüksek öncelikli görevi seçer
 * 3. Görevi 1 saniye çalıştırır
 * 4. Görev yönetimi yapar (askıya alma, sonlandırma, öncelik düşürme)
 * 5. Zamanı ilerletir ve zamanaSIMI kontrolü yapar
 */
void SchedulerTask(void *pvParameters) {
    TaskInfo* currentTask = NULL;
    
    for (;;) {
        // ===== ADIM 1: YENÄ° GELENLERÄ° KUYRUKLARA YERLEÅŸTÄ°R =====
        for (int i = 0; i < pendingCount; i++) {
            if (pendingTasks[i] != NULL && pendingTasks[i]->arrivalTime == globalTime) {
                TaskInfo* t = pendingTasks[i];
                
                // Önceliğe göre uygun kuyruğa yerleştir
                if (t->priority == 0) {
                    enqueue(&qRT, t);  // Gerçek Zamanlı
                } else if (t->priority == 1) {
                    enqueue(&qP1, t);  // Yüksek öncelikli kullanıcı
                } else if (t->priority == 2) {
                    enqueue(&qP2, t);  // Orta öncelikli kullanıcı
                } else {
                    enqueue(&qP3, t);  // Düşük öncelikli kullanıcı (3 ve üzeri)
                }
                
                pendingTasks[i] = NULL;
            }
        }

        // ===== ADIM 2: ÇALIÅžTIRILACAK GÖREVİ SEÃ‡ (ÃœÃ‡ SEVÄ°YELÄ° GERİ BESLEMELÄ° KUYRUK) =====
        TaskInfo* nextTask = NULL;

        // Öncelik sırası: RT > P1 > P2 > P3
        if (qRT.head != NULL) {
            nextTask = qRT.head;
        } else if (qP1.head != NULL) {
            nextTask = qP1.head;
        } else if (qP2.head != NULL) {
            nextTask = qP2.head;
        } else if (qP3.head != NULL) {
            nextTask = qP3.head;
        }

        // ===== ADIM 3: GÖREVİ ÃœRÃœT (1 SANİYE) =====
        if (nextTask != NULL) {
            currentTask = nextTask;
            
            // Çalışan görevin bekleme sayacını sıfırla
            currentTask->waitCounter = 0;

            // --- Görev Durumu Mesajı ---
            if (currentTask->hasStarted == 0) {
                // İlk kez başlıyor
                printf("%s%d.0000 sn\tproses başladı\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       currentTask->color, globalTime, currentTask->id, 
                       currentTask->priority, currentTask->remainingTime, COLOR_RESET);
                currentTask->hasStarted = 1;
            } else {
                // Devam ediyor (askıdan dönmüş veya RT görevi)
                printf("%s%d.0000 sn\tproses yürütülüyor\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                       currentTask->color, globalTime, currentTask->id, 
                       currentTask->priority, currentTask->remainingTime, COLOR_RESET);
            }

            // 1 saniye işlem yap (kalan süreyi azalt)
            currentTask->remainingTime--;

            // ===== ADIM 4: GÖREV YÖNETÄ°MÄ° (TAMAMLANMA / ASKIYA ALMA) =====
            if (currentTask->remainingTime == 0) {
                // --- GÖREV TAMAMLANDI ---
                printf("%s%d.0000 sn\tproses sonlandı\t\t(id:%04d\töncelik:%d\tkalan süre:0 sn)%s\n",
                       currentTask->color, globalTime + 1, currentTask->id, 
                       currentTask->priority, COLOR_RESET);
                
                currentTask->isFinished = 1;
                
                // Kuyruktan çıkar
                if (currentTask->priority == 0) {
                    dequeue(&qRT);
                } else if (currentTask->priority == 1) {
                    dequeue(&qP1);
                } else if (currentTask->priority == 2) {
                    dequeue(&qP2);
                } else {
                    dequeue(&qP3);
                }
                
                vTaskDelete(currentTask->handle);
                
            } else {
                // --- GÖREV HENÜZ BİTMEDİ ---
                
                if (currentTask->priority == 0) {
                    // **RT GÖREVİ**: Kesintisiz devam eder, kuyrukta kalır
                    // Sonraki saniyede yine başta olacak ve çalışmaya devam edecek
                    
                } else {
                    // **KULLANICI GÖREVİ**: 1 saniye çalıştı, şimdi askıya alınacak
                    
                    // Mevcut kuyruktan çıkar
                    if (currentTask->priority == 1) {
                        dequeue(&qP1);
                    } else if (currentTask->priority == 2) {
                        dequeue(&qP2);
                    } else {
                        dequeue(&qP3);
                    }
                    
                    // Öncelik düşür (değer artar: 1->2, 2->3, 3->4, ...)
                    // Not: Öncelik 3'te kilitlenmez, 4, 5, 6... olabilir ama hepsi P3 kuyruğuna gider
                    currentTask->priority++;
                    
                    // Askıya alma mesajı (bir sonraki saniyenin başında)
                    printf("%s%d.0000 sn\tproses askıda\t\t(id:%04d\töncelik:%d\tkalan süre:%d sn)%s\n",
                           currentTask->color, globalTime + 1, currentTask->id, 
                           currentTask->priority, currentTask->remainingTime, COLOR_RESET);
                    
                    // Yeni önceliğe göre uygun kuyruğa ekle
                    if (currentTask->priority == 1) {
                        enqueue(&qP1, currentTask);  // Teorik (1'den başlayanlar buraya gelmez)
                    } else if (currentTask->priority == 2) {
                        enqueue(&qP2, currentTask);  // 1'den 2'ye düştü
                    } else {
                        enqueue(&qP3, currentTask);  // 2'den 3'e veya 3'ten üste (hepsi P3'te)
                    }
                }
            }
        }

        // ===== ADIM 5: ZAMANI Ä°LERLET VE ZAMANASIMI KONTROL ET =====
        globalTime++;
        CheckTimeouts();

        // ===== SÄ°MÃœLASYON BÄ°TÄ°ÅžÄ° KONTROL =====
        int allDone = 1;
        
        // Bekleyen görev var mı?
        for (int k = 0; k < MAX_TASKS; k++) {
            if (pendingTasks[k] != NULL) {
                allDone = 0;
                break;
            }
        }
        
        // Kuyruklarda görev var mı?
        if (qRT.head || qP1.head || qP2.head || qP3.head) {
            allDone = 0;
        }
        
        if (allDone && globalTime > 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            exit(0);
        }
        
        // 1 saniye bekle (gerçek zaman simülasyonu)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}