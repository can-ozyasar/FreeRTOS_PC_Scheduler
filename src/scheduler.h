/* src/scheduler.h */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// ========== RENK KODLARI (ANSI Escape Codes) ==========
// Her görev için farklı renk kullanılarak terminal çıktısında ayırt edilebilirlik sağlanır
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

// ========== GÖREV BİLGİ YAPISI ==========
/**
 * Her görevin durumunu ve özelliklerini tutan veri yapısı.
 * Bağlı liste (linked list) olarak kuyruk yapısında kullanılır.
 */
typedef struct TaskInfo {
    int id;                     // Görev kimlik numarası (giris.txt'deki sıra)
    int arrivalTime;            // Görevin sisteme geldiği zaman (saniye)
    int priority;               // Öncelik değeri (0: RT, 1-3+: Kullanıcı)
                                // Düşük değer = Yüksek öncelik
                                // 0: Gerçek Zamanlı (FCFS, kesintisiz)
                                // 1: Yüksek öncelikli kullanıcı görevi
                                // 2: Orta öncelikli kullanıcı görevi
                                // 3+: Düşük öncelikli kullanıcı görevi (sınır yok)
    int burstTime;              // Toplam iş süresi (saniye)
    int remainingTime;          // Kalan iş süresi (saniye)
    int waitCounter;            // Bekleme sayacı (20 saniye kuralı için)
                                // Her saniye +1 artar, çalıştığında sıfırlanır
                                // >= 20 olursa zamanaSIMI
    TaskHandle_t handle;        // FreeRTOS görev tanıtıcısı
    const char* color;          // Terminal renk kodu
    int isFinished;             // Görev tamamlandı mı? (1: Evet, 0: Hayır)
    int hasStarted;             // Görev daha önce başladı mı? (1: Evet, 0: Hayır)
                                // İlk çalıştırmada "başladı", sonrasında "yürütülüyor"
    struct TaskInfo* next;      // Sonraki görev (bağlı liste için)
} TaskInfo;

// ========== KUYRUK YAPISI ==========
/**
 * FIFO (First-In-First-Out) kuyruk yapısı.
 * Görevlendirici dört ayrı kuyruk kullanır: RT, P1, P2, P3
 */
typedef struct {
    TaskInfo* head;             // Kuyruğun başı (ilk çalışacak görev)
    TaskInfo* tail;             // Kuyruğun sonu (en son eklenen görev)
    int count;                  // Kuyruktaki görev sayısı
} Queue;

// ========== GLOBAL KUYRUK DEĞİŞKENLERİ ==========
/**
 * ÜÇ SEVİYELİ GERİ BESLEMELI GÖREVLENDİRİCİ + GERÇEK ZAMANLI KUYRUK
 * 
 * Öncelik Sırası (yüksekten düşüğe):
 * 1. qRT  - Gerçek Zamanlı Kuyruk (öncelik 0)
 *           FCFS, kesintisiz çalışır, zamanaSIMI OLMAZ
 * 
 * 2. qP1  - Yüksek Öncelikli Kullanıcı Kuyruk (öncelik 1)
 *           1 saniye çalışır, sonra öncelik 2'ye düşer ve P2'ye gider
 * 
 * 3. qP2  - Orta Öncelikli Kullanıcı Kuyruk (öncelik 2)
 *           1 saniye çalışır, sonra öncelik 3'e düşer ve P3'e gider
 * 
 * 4. qP3  - Düşük Öncelikli Kullanıcı Kuyruk (öncelik 3 ve üzeri)
 *           1 saniye çalışır, öncelik 4, 5, 6... olabilir ama hepsi P3'te kalır
 *           Round-Robin mantığıyla çalışır
 * 
 * NOT: Tüm kullanıcı kuyrukları 20 saniye bekleme kuralına tabidir.
 *      20 saniye bekleyen görevler zamanaSIMI olur.
 */
extern Queue qRT;   // Öncelik 0: Gerçek Zamanlı Kuyruk
extern Queue qP1;   // Öncelik 1: Yüksek Öncelikli Kullanıcı Kuyruk
extern Queue qP2;   // Öncelik 2: Orta Öncelikli Kullanıcı Kuyruk
extern Queue qP3;   // Öncelik 3+: Düşük Öncelikli Kullanıcı Kuyruk (Generic)

// ========== FONKSİYON PROTOTÄ°PLERÄ° ==========

/**
 * Görevlendiriciyi başlatır ve görev listesini dosyadan okur.
 * @param filename Görev listesi dosyası (örn: giris.txt)
 */
void SchedulerInit(const char* filename);

/**
 * Ana görevlendirici görevi.
 * Her saniye:
 * - Yeni gelen görevleri kuyruklara yerleştirir
 * - En yüksek öncelikli görevi seçer ve çalıştırır
 * - Görev yönetimi yapar (askıya alma, sonlandırma, öncelik düşürme)
 * - Zamanı ilerletir ve zamanaSIMI kontrolü yapar
 */
void SchedulerTask(void *pvParameters);

/**
 * İşçi görev fonksiyonu.
 * Gerçekte çalışmaz, sadece FreeRTOS için placeholder olarak var olur.
 * Tüm yönetim Scheduler tarafından yapılır.
 */
void WorkerTask(void *pvParameters);

#endif // SCHEDULER_H