# 4 SEVİYELİ ÖNCELİKLİ GÖREVLENDİRİCİ ALGORITMA DOKÜMANTASYONU

## 📐 Algoritma Genel Bakış

Bu görevlendirici, **hibrit bir yaklaşım** kullanır:
1. **FCFS (First-Come-First-Served)** - Gerçek zamanlı görevler için
2. **Çok Seviyeli Geri Beslemeli Kuyruk** - Kullanıcı görevleri için
3. **Round-Robin** - En düşük öncelik seviyesinde

## 🔄 Ana Algoritma Akışı

### Pseudocode (Sözde Kod)

```plaintext
BAŞLAT Görevlendirici
    globalTime ← 0
    Kuyrukları başlat: qRT, qP1, qP2, qP3
    Görev listesini oku (giris.txt)
    
    HER SANİYE TEKRARLA:
        ┌─────────────────────────────────────────────────────┐
        │ ADIM 1: YENİ GÖREVLERİ KUYRUKLARA YERLEŞTİR        │
        └─────────────────────────────────────────────────────┘
        HER görev İÇİN görev_listesi:
            EĞER görev.arrivalTime == globalTime İSE:
                EĞER görev.priority == 0 İSE:
                    qRT'ye ekle (Gerçek Zamanlı)
                DEĞİLSE EĞER görev.priority == 1 İSE:
                    qP1'e ekle (Yüksek Öncelik)
                DEĞİLSE EĞER görev.priority == 2 İSE:
                    qP2'ye ekle (Orta Öncelik)
                DEĞİLSE:
                    qP3'e ekle (Düşük Öncelik, 3 ve üzeri)
        
        ┌─────────────────────────────────────────────────────┐
        │ ADIM 2: ÇALIŞTIRILABİLİR GÖREVİ SEÇ               │
        └─────────────────────────────────────────────────────┘
        nextTask ← NULL
        
        EĞER qRT.head ≠ NULL İSE:
            nextTask ← qRT.head          // En yüksek öncelik
        DEĞİLSE EĞER qP1.head ≠ NULL İSE:
            nextTask ← qP1.head
        DEĞİLSE EĞER qP2.head ≠ NULL İSE:
            nextTask ← qP2.head
        DEĞİLSE EĞER qP3.head ≠ NULL İSE:
            nextTask ← qP3.head
        
        ┌─────────────────────────────────────────────────────┐
        │ ADIM 3: SEÇİLEN GÖREVİ ÇALIŞTIR (q = 1 sn)        │
        └─────────────────────────────────────────────────────┘
        EĞER nextTask ≠ NULL İSE:
            currentTask ← nextTask
            currentTask.waitCounter ← 0    // Çalıştığı için sıfırla
            
            // Durum mesajı yazdır
            EĞER currentTask.hasStarted == FALSE İSE:
                YAZDIR("başladı")
                currentTask.hasStarted ← TRUE
            DEĞİLSE:
                YAZDIR("yürütülüyor")
            
            // 1 saniye işlem yap
            currentTask.remainingTime ← currentTask.remainingTime - 1
            
            ┌─────────────────────────────────────────────────┐
            │ ADIM 4: GÖREV YÖNETİMİ                         │
            └─────────────────────────────────────────────────┘
            EĞER currentTask.remainingTime == 0 İSE:
                // GÖREV TAMAMLANDI
                YAZDIR("sonlandı" @ globalTime+1)
                Kuyruktan çıkar
                Görevi sil
            
            DEĞİLSE:
                // GÖREV HENÜZ BİTMEDİ
                EĞER currentTask.priority == 0 İSE:
                    // GERÇEK ZAMANLI GÖREV
                    // Kesintisiz devam et, kuyrukta kal
                    
                DEĞİLSE:
                    // KULLANICI GÖREVİ
                    // Askıya al ve öncelik düşür
                    Mevcut kuyruktan çıkar
                    currentTask.priority ← currentTask.priority + 1
                    YAZDIR("askıda" @ globalTime+1)
                    
                    // Yeni kuyruğa ekle
                    EĞER currentTask.priority == 1 İSE:
                        qP1'e ekle
                    DEĞİLSE EĞER currentTask.priority == 2 İSE:
                        qP2'ye ekle
                    DEĞİLSE:
                        qP3'e ekle  // 3, 4, 5, ... hepsi buraya
        
        ┌─────────────────────────────────────────────────────┐
        │ ADIM 5: ZAMANI İLERLET VE ZAMANASIMI KONTROL ET   │
        └─────────────────────────────────────────────────────┘
        globalTime ← globalTime + 1
        
        // Her kullanıcı kuyruğundaki bekleyen görevler için
        HER kuyruk İÇİN [qP1, qP2, qP3]:
            HER görev İÇİN kuyruk:
                görev.waitCounter ← görev.waitCounter + 1
                
                EĞER görev.waitCounter >= 20 İSE:
                    YAZDIR("zamanaSIMI" @ globalTime)
                    Kuyruktan çıkar
                    Görevi sil
        
        ┌─────────────────────────────────────────────────────┐
        │ ADIM 6: SİMÜLASYON BİTİŞİ KONTROL ET              │
        └─────────────────────────────────────────────────────┘
        EĞER (görev_listesi boş) VE (tüm kuyruklar boş) İSE:
            SİMÜLASYONU SONLANDIR
        
        1 saniye bekle
    DÖNGÜ SONU
SONLANDIR
```

## 🎯 Kritik Algoritma Detayları

### 1. Öncelik Değeri ve Kuyruk İlişkisi

```
Öncelik Değeri (düşük = yüksek öncelik)    Kuyruk        Açıklama
─────────────────────────────────────────────────────────────────────
        0                                    qRT        Gerçek Zamanlı
        1                                    qP1        Yüksek Öncelik
        2                                    qP2        Orta Öncelik
      3, 4, 5, 6, ...                        qP3        Düşük Öncelik
```

**Önemli**: Öncelik 3'ten sonra sınır yoktur. Görev 4, 5, 6... olabilir ama hepsi **qP3 kuyruğunda** tutulur ve Round-Robin ile çalışır.

### 2. Geri Besleme Mekanizması

```
┌─────────────────────────────────────────────────────────────────┐
│                 GERİ BESLEMELI KUYRUK SİSTEMİ                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Görev 1. kez gelir → priority = 1                             │
│         ↓                                                       │
│  qP1'de başlar (yüksek öncelik)                                │
│         ↓ 1 sn çalışır                                         │
│  Askıya alınır → priority = 2                                  │
│         ↓                                                       │
│  qP2'ye eklenir (orta öncelik)                                 │
│         ↓ sırası gelir, 1 sn çalışır                          │
│  Askıya alınır → priority = 3                                  │
│         ↓                                                       │
│  qP3'e eklenir (düşük öncelik)                                 │
│         ↓ sırası gelir, 1 sn çalışır                          │
│  Askıya alınır → priority = 4                                  │
│         ↓                                                       │
│  qP3'e YİNE eklenir (kuyruğun sonuna)                          │
│         ↓ Round-Robin ile döner                                │
│  priority = 5, 6, 7, ... (sonsuza kadar artabilir)            │
│                                                                 │
│  NOT: priority arttıkça görevin gerçek önceliği AZALIR        │
│       (Büyük sayı = düşük öncelik)                             │
└─────────────────────────────────────────────────────────────────┘
```

### 3. ZamanaSIMI Algoritması

```
┌──────────────────────────────────────────────────────────────┐
│              20 SANİYE BEKLEME KURALI                        │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  waitCounter başlangıç: 0                                   │
│                                                              │
│  HER SANİYE:                                                │
│      EĞER görev çalıştırılıyorsa:                           │
│          waitCounter ← 0  (sıfırla)                         │
│      DEĞİLSE (kuyrukta bekliyor):                           │
│          waitCounter ← waitCounter + 1                      │
│                                                              │
│  EĞER waitCounter >= 20 İSE:                                │
│      "zamanaSIMI" mesajı yazdır                             │
│      Görevi sonlandır                                       │
│                                                              │
│  ÖNEMLİ: RT kuyruk (öncelik 0) için zamanaSIMI YOKTUR!     │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│                          ÖRNEK                               │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  0. sn:  Görev gelir, qP1'e eklenir   → waitCounter = 0    │
│  1. sn:  Bekliyor (çalışmadı)         → waitCounter = 1    │
│  2. sn:  Bekliyor                      → waitCounter = 2    │
│  3. sn:  Çalıştı! (1 sn)               → waitCounter = 0    │
│  4. sn:  Askıda, qP2'de bekliyor       → waitCounter = 1    │
│  5. sn:  Bekliyor                      → waitCounter = 2    │
│  ...                                                         │
│  23. sn: Bekliyor                      → waitCounter = 19   │
│  24. sn: Bekliyor                      → waitCounter = 20   │
│  25. sn: ZamanaSIMI! → Sonlandırılır                        │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

### 4. Gerçek Zamanlı Görev Yönetimi

RT görevleri (öncelik 0) **özel bir statüye** sahiptir:

```
┌────────────────────────────────────────────────────────┐
│           GERÇEK ZAMANLI GÖREV ÖZELLİKLERİ            │
├────────────────────────────────────────────────────────┤
│                                                        │
│  ✓ En yüksek öncelik (diğer tüm görevleri keser)     │
│  ✓ FCFS (İlk gelen ilk çalışır)                      │
│  ✓ Kesintisiz çalışır (preemption yok)               │
│  ✓ ZamanaSIMI OLMAZ                                   │
│  ✓ Öncelik DEĞİŞMEZ (hep 0 kalır)                    │
│  ✓ Kuyruktan çıkarılmaz, tamamlanana kadar devam eder│
│                                                        │
├────────────────────────────────────────────────────────┤
│                    ÇALIŞMA AKIŞI                       │
├────────────────────────────────────────────────────────┤
│                                                        │
│  qRT'ye ekle                                          │
│     ↓                                                  │
│  Sırası gelir → "başladı" mesajı                      │
│     ↓                                                  │
│  1 sn çalış → remainingTime--                         │
│     ↓                                                  │
│  Bitti mi? ───── EVET → "sonlandı", kuyruktan çıkar  │
│     │                                                  │
│    HAYIR                                              │
│     ↓                                                  │
│  "yürütülüyor" mesajı                                 │
│     ↓                                                  │
│  1 sn çalış → remainingTime--                         │
│     ↓                                                  │
│  (Tamamlanana kadar devam eder...)                    │
│                                                        │
└────────────────────────────────────────────────────────┘
```

## 📊 Veri Yapıları ve Bellek Yönetimi

### Görev Bilgi Yapısı (TaskInfo)

```c
struct TaskInfo {
    // === SABIT BİLGİLER (başlangıçta ayarlanır) ===
    int id;              // Görev kimliği (0, 1, 2, ...)
    int arrivalTime;     // Geldiği zaman (saniye)
    int burstTime;       // Toplam iş süresi (saniye)
    const char* color;   // Terminal renk kodu
    
    // === DİNAMİK BİLGİLER (çalışırken değişir) ===
    int priority;        // Öncelik (0: RT, 1-3+: Kullanıcı)
                        // ↑ Her askıya alınmada +1 artar
    int remainingTime;   // Kalan iş süresi (saniye)
                        // ↓ Her çalıştırmada -1 azalır
    int waitCounter;     // Bekleme sayacı (20 sn kuralı için)
                        // ↑ Beklerken +1, çalışırken 0'lanır
    
    // === DURUM BİLGİLERİ ===
    int hasStarted;      // 0: Hiç başlamadı, 1: En az 1 kez başladı
    int isFinished;      // 0: Devam ediyor, 1: Tamamlandı/Sonlandırıldı
    
    // === FREERTOS & BAĞLI LİSTE ===
    TaskHandle_t handle; // FreeRTOS görev tanıtıcısı
    struct TaskInfo* next; // Sonraki görev (kuyruk için)
};
```

### Kuyruk Yapısı (Queue)

```
┌─────────────────────────────────────────────────────────┐
│                    FIFO KUYRUK YAPISI                   │
├─────────────────────────────────────────────────────────┤
│                                                         │
│   head                               tail               │
│     ↓                                  ↓                │
│  [Task1] → [Task2] → [Task3] → [Task4]                 │
│    │                                   ↑                │
│    ↓                                   │                │
│  Çıkış                               Giriş             │
│ (dequeue)                           (enqueue)          │
│                                                         │
│  count = 4                                              │
│                                                         │
├─────────────────────────────────────────────────────────┤
│                     İŞLEMLER                            │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  enqueue(kuyruk, görev):                                │
│      görev.next ← NULL                                  │
│      tail.next ← görev                                  │
│      tail ← görev                                       │
│      count++                                            │
│                                                         │
│  dequeue(kuyruk):                                       │
│      temp ← head                                        │
│      head ← head.next                                   │
│      count--                                            │
│      DÖNDÜR temp                                        │
│                                                         │
│  remove_from_queue(kuyruk, görev):                      │
│      Görevi bul ve listeden çıkar                       │
│      Bağlantıları yeniden düzenle                       │
│      count--                                            │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

## 🔄 Durum Geçişleri

```
┌────────────────────────────────────────────────────────────────┐
│                    GÖREV DURUM DİYAGRAMI                       │
└────────────────────────────────────────────────────────────────┘

           [YENİ GÖREV]
                ↓
         arrivalTime geldi
                ↓
    ┌───────────────────────────┐
    │      KUYRUĞA EKLENDİ      │  ← Önceliğe göre uygun kuyruk
    │   (READY - Hazır Bekliyor) │
    └───────────────────────────┘
                ↓
         Sırası geldi
                ↓
    ┌───────────────────────────┐
    │    ÇALIÅžIYOR (RUNNING)    │  ← "başladı" / "yürütülüyor"
    │     q = 1 saniye          │
    └───────────────────────────┘
                ↓
          remainingTime--
                ↓
        ┌────────────────┐
        │ Bitti mi?      │
        └────────────────┘
         │            │
        EVET         HAYIR
         │            │
         │            └──────────┐
         │                       │
         │                  RT mi?
         │                       │
         │              EVET ────┴──── HAYIR
         │               │               │
         ↓               ↓               ↓
    ┌─────────┐   ┌──────────┐   ┌──────────────┐
    │SONLANDI │   │DEVam     │   │ ASKIDA       │
    │(DONE)   │   │eder      │   │ (SUSPENDED)  │
    └─────────┘   └──────────┘   └──────────────┘
                       ↑               │
                       │               │ priority++
                       │               │
                       │               ↓
                       │       Yeni kuyruğa ekle
                       │               │
                       └───────────────┘

                [ZAMANASIMI]
                      ↑
                      │
         waitCounter >= 20
              (20 sn beklediyse)
```

## ⏱️ Zaman Kompleksitesi Analizi

### İşlem Başına Karmaşıklık

| İşlem | Zaman Karmaşıklığı | Açıklama |
|-------|-------------------|----------|
| `enqueue()` | O(1) | Kuyruğun sonuna ekleme (sabit zaman) |
| `dequeue()` | O(1) | Kuyruğun başından çıkarma (sabit zaman) |
| `remove_from_queue()` | O(n) | Belirli bir görev aranır (n = kuyruk uzunluğu) |
| `CheckTimeouts()` | O(n) | Tüm görevler taranır (n = toplam görev sayısı) |
| Görev Seçimi | O(1) | 4 kuyruğun başı kontrol edilir |
| Yeni Görev Ekleme | O(m) | m = bekleyen görev sayısı |

### Her Saniye Toplam Karmaşıklık

```
T(n) = O(m) + O(1) + O(1) + O(n) + O(1)
     = O(n + m)
     
n = Kuyruklardaki görev sayısı
m = Bekleyen görev sayısı
```

**Pratik Performans**: Görev sayısı genellikle küçük olduğu için (< 100), algoritma çok hızlı çalışır.

## 🎨 Mesaj Zamanlaması

Mesajların hangi zamanda yazdırıldığı kritiktir:

```
┌─────────────────────────────────────────────────────────┐
│                  MESAJ ZAMANLAMA KURALLARI              │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Mesaj Türü          Yazdırılma Zamanı    Açıklama    │
│  ─────────────────────────────────────────────────────  │
│  "başladı"          globalTime             İlk kez     │
│                                            başlarken    │
│                                                         │
│  "yürütülüyor"      globalTime             Devam       │
│                                            ederken      │
│                                                         │
│  "askıda"           globalTime + 1         Bir sonraki │
│                                            sn başında   │
│                                                         │
│  "sonlandı"         globalTime + 1         Bir sonraki │
│                                            sn başında   │
│                                                         │
│  "zamanaSIMI"       globalTime             Timeout     │
│                                            anında       │
│                                                         │
└─────────────────────────────────────────────────────────┘

ÖRNEK AKIŞ:

0. sn: Görev başlar → "başladı" @ 0. sn
       Çalışır (1 sn)
       
1. sn: Eğer bitmemişse → "askıda" @ 1. sn
       (Mesaj önce yazdırılır, sonra globalTime++)
       
Kod:
    printf("askıda @ %d", globalTime + 1);  // Henüz artmadı
    globalTime++;                           // Şimdi arttı
```

## 🔍 Algoritma Doğrulama

### Test Senaryoları

#### Test 1: Tek RT Görev
```
Girdi: 0, 0, 3

Beklenen Çıktı:
0.0000 sn    proses başladı        (id:0000 öncelik:0 kalan süre:3 sn)
1.0000 sn    proses yürütülüyor    (id:0000 öncelik:0 kalan süre:2 sn)
2.0000 sn    proses yürütülüyor    (id:0000 öncelik:0 kalan süre:1 sn)
3.0000 sn    proses sonlandı       (id:0000 öncelik:0 kalan süre:0 sn)
```

#### Test 2: Kullanıcı Görevi Öncelik Düşüşü
```
Girdi: 0, 1, 3

Beklenen Çıktı:
0.0000 sn    proses başladı        (id:0000 öncelik:1 kalan süre:3 sn)
1.0000 sn    proses askıda         (id:0000 öncelik:2 kalan süre:2 sn)
1.0000 sn    proses yürütülüyor    (id:0000 öncelik:2 kalan süre:2 sn)
2.0000 sn    proses askıda         (id:0000 öncelik:3 kalan süre:1 sn)
2.0000 sn    proses yürütülüyor    (id:0000 öncelik:3 kalan süre:1 sn)
3.0000 sn    proses sonlandı       (id:0000 öncelik:3 kalan süre:0 sn)
```

#### Test 3: RT Görev Önceliği
```
Girdi: 
0, 1, 2  (Kullanıcı görevi)
1, 0, 1  (RT görev, öncelikli)

Beklenen Sonuç:
- 0. sn: Kullanıcı görevi başlar
- 1. sn: RT görev gelir, HEMEN kesme yapar, önce RT tamamlanır
- 2. sn: Kullanıcı görevi devam eder
```

## 📚 Gerçek Dünya Karşılaştırmaları

### Linux CFS (Completely Fair Scheduler) ile Karşılaştırma

| Özellik | Bu Proje | Linux CFS |
|---------|----------|-----------|
| Öncelik Seviyeleri | 4 (0-3+) | 140 (0-139) |
| Gerçek Zamanlı | FCFS | FIFO, RR, Deadline |
| Kullanıcı Görevleri | Geri Beslemeli | Adil Zaman Paylaşımı |
| Zaman Kuantumu | 1 sn (sabit) | Dinamik (nice değerine göre) |
| Açlık Önleme | 20 sn timeout | vruntime dengeleme |

### Windows Thread Scheduler ile Karşılaştırma

| Özellik | Bu Proje | Windows |
|---------|----------|---------|
| Öncelik Seviyeleri | 4 | 32 |
| Dinamik Öncelik | Evet (düşer) | Evet (boost) |
| Gerçek Zamanlı | Var (FCFS) | Var (16-31) |
| Round-Robin | En düşük seviye | Her seviyede |

## 🎯 Algoritma Avantajları ve Dezavantajları

### ✅ Avantajlar

1. **Basitlik**: Anlaşılması ve uygulanması kolay
2. **Deterministik**: RT görevler için öngörülebilir davranış
3. **Adalet**: Geri besleme ile uzun görevler cezalandırılır
4. **Açlık Önleme**: 20 sn timeout ile hiçbir görev sonsuza kadar beklemez

### ❌ Dezavantajlar

1. **Sabit Kuantum**: 1 sn çok uzun veya çok kısa olabilir
2. **Sınırlı Öncelik**: Sadece 4 seviye (gerçek sistemlerde 32-140 seviye var)
3. **Basit Timeout**: 20 sn kuralı katı, dinamik değil
4. **Context Switch**: Her görev değişiminde overhead

### 🔧 Önerilen İyileştirmeler

1. **Dinamik Kuantum**: 
   - P1: 1 sn
   - P2: 2 sn
   - P3: 4 sn (daha uzun görevler için)

2. **Öncelik Boost**:
   - Çok bekleyen görevlerin önceliği artırılabilir (değer azalır)
   - Açlık önleme daha etkin olur

3. **Dinamik Timeout**:
   - Önceliğe göre farklı timeout değerleri
   - RT: Timeout yok
   - P1: 30 sn
   - P2: 20 sn
   - P3: 10 sn

4. **İstatistik Takibi**:
   - Bekleme süresi ortalaması
   - Turnaround time
   - CPU kullanım oranı

## 📈 Performans Metrikleri

### Hesaplanabilecek Metrikler

```
1. Bekleme Süresi (Waiting Time):
   WT = Tamamlanma Zamanı - Varış Zamanı - İş Süresi

2. Turnaround Süresi:
   TAT = Tamamlanma Zamanı - Varış Zamanı

3. Tepki Süresi (Response Time):
   RT = İlk Çalışma Zamanı - Varış Zamanı

4. CPU Kullanımı:
   CPU = (Toplam İş Süresi / Toplam Geçen Zaman) × 100%

5. Throughput:
   TP = Tamamlanan Görev Sayısı / Toplam Zaman
```

Bu algoritma dokümantasyonu, raporunuzda kullanabileceğiniz detaylı bir kaynak sağlar. Her bölüm, farklı açılardan algoritmanın nasıl çalıştığını açıklar.