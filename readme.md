# FreeRTOS Görevlendirici Simülasyonu

## 📋 Proje Açıklaması

Bu proje, **4 seviyeli öncelikli görevlendirici** simülasyonunu FreeRTOS kullanarak PC ortamında (POSIX) gerçekleştirir. 

### Görevlendirici Yapısı

```
┌─────────────────────────────────────────────────────┐
│         4 SEVİYELİ ÖNCELİKLİ GÖREVLENDİRİCİ          │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌──────────────────────────────────────────────┐  │
│  │  Öncelik 0: Gerçek Zamanlı Kuyruk (qRT)     │  │
│  │  • FCFS (İlk Gelen İlk Çalışır)             │  │
│  │  • Kesintisiz çalışır                       │  │
│  │  • ZamanaSIMI OLMAZ                         │  │
│  └──────────────────────────────────────────────┘  │
│                       ↓ (RT boşsa)                  │
│  ┌──────────────────────────────────────────────┐  │
│  │  3 SEVİYELİ GERİ BESLEMELI KUYRUK YAPISI    │  │
│  │                                              │  │
│  │  ┌────────────────────────────────────────┐ │  │
│  │  │ Öncelik 1: qP1 (Yüksek Öncelik)       │ │  │
│  │  │ q = 1 saniye                           │ │  │
│  │  └────────────────────────────────────────┘ │  │
│  │                    ↓ (askıya alma)           │  │
│  │  ┌────────────────────────────────────────┐ │  │
│  │  │ Öncelik 2: qP2 (Orta Öncelik)         │ │  │
│  │  │ q = 1 saniye                           │ │  │
│  │  └────────────────────────────────────────┘ │  │
│  │                    ↓ (askıya alma)           │  │
│  │  ┌────────────────────────────────────────┐ │  │
│  │  │ Öncelik 3+: qP3 (Düşük Öncelik)       │ │  │
│  │  │ q = 1 saniye, Round-Robin             │ │  │
│  │  │ (3, 4, 5, ... hepsi burada)           │ │  │
│  │  └────────────────────────────────────────┘ │  │
│  │                                              │  │
│  │  ** 20 SANİYE BEKLEME KURALI **             │  │
│  │  Kuyrukta 20 sn bekleyen → ZamanaSIMI       │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

## 🎯 Algoritma Detayları

### 1. Görev Geliş ve Kuyruk Yerleştirme
- Her saniye `giris.txt` dosyasından gelen görevler kontrol edilir
- Görevler önceliklerine göre uygun kuyruğa yerleştirilir:
  - `öncelik = 0` → **qRT** (Gerçek Zamanlı)
  - `öncelik = 1` → **qP1** (Yüksek Öncelikli Kullanıcı)
  - `öncelik = 2` → **qP2** (Orta Öncelikli Kullanıcı)
  - `öncelik ≥ 3` → **qP3** (Düşük Öncelikli Kullanıcı)

### 2. Görev Seçimi (Öncelik Sırası)
```
qRT boş değil mi? → RT görevini seç (en yüksek öncelik)
   ↓ hayır
qP1 boş değil mi? → P1 görevini seç
   ↓ hayır
qP2 boş değil mi? → P2 görevini seç
   ↓ hayır
qP3 boş değil mi? → P3 görevini seç
   ↓ hayır
Boşta bekle (idle)
```

### 3. Görev Çalıştırma ve Yönetimi

#### **Gerçek Zamanlı Görevler (Öncelik 0)**
```
Başla → Çalışır (q=1sn) → remainingTime--
                ↓
        Bitti mi? ─── EVET → Sonlandır
                │
               HAYIR → Devam et (kesintisiz)
```
- **Kesintisiz çalışır** (preemption yok)
- Tamamlanana kadar çalışmaya devam eder
- ZamanaSIMI **OLMAZ**

#### **Kullanıcı Görevleri (Öncelik 1, 2, 3+)**
```
Başla → Çalışır (q=1sn) → remainingTime--
                ↓
        Bitti mi? ─── EVET → Sonlandır
                │
               HAYIR → Askıya al
                       priority++
                       Yeni kuyruğa ekle
                       
Örnek Akış:
P1'den başla (öncelik=1)
  ↓ 1 sn çalış
Askıya al, öncelik=2 yap, P2'ye ekle
  ↓ Sırası gelince
P2'den devam et (öncelik=2)
  ↓ 1 sn çalış
Askıya al, öncelik=3 yap, P3'e ekle
  ↓ Sırası gelince
P3'te devam et (öncelik=3)
  ↓ 1 sn çalış
Askıya al, öncelik=4 yap, yine P3'e ekle (Round-Robin)
```

### 4. ZamanaSIMI Kuralı (20 Saniye)
- **Sadece kullanıcı kuyrukları** (P1, P2, P3) için geçerlidir
- Her saniye, çalışmayan görevlerin `waitCounter`'ı +1 artar
- Çalışan görevin `waitCounter`'ı sıfırlanır
- `waitCounter ≥ 20` → ZamanaSIMI → Görev sonlandırılır

```
Örnek:
0. sn: Görev gelir (waitCounter=0)
1. sn: Bekliyor (waitCounter=1)
2. sn: Bekliyor (waitCounter=2)
...
20. sn: Bekliyor (waitCounter=20)
21. sn: ZamanaSIMI! → Sonlandır
```

## 📊 Çıktı Formatı

Her saniye aşağıdaki mesajlardan biri yazdırılır:

```
X.0000 sn    proses başladı        (id:XXXX öncelik:X kalan süre:X sn)
X.0000 sn    proses yürütülüyor    (id:XXXX öncelik:X kalan süre:X sn)
X.0000 sn    proses askıda         (id:XXXX öncelik:X kalan süre:X sn)
X.0000 sn    proses sonlandı       (id:XXXX öncelik:X kalan süre:0 sn)
X.0000 sn    proses zamanaSIMI     (id:XXXX öncelik:X kalan süre:X sn)
```

Her görev farklı renkle gösterilir (ANSI renk kodları).

## 🚀 Derleme ve Çalıştırma

### Gereksinimler
- **İşletim Sistemi**: Linux, macOS veya Windows (WSL/MinGW)
- **Derleyici**: gcc
- **FreeRTOS**: Kernel kaynak kodu (proje klasöründe mevcut)

### Derleme
```bash
cd FreeRTOS_PC_Scheduler
make
```

### Çalıştırma
```bash
./freertos_sim giris.txt
```

### Temizleme
```bash
make clean
```

## 📁 Proje Yapısı

```
FreeRTOS_PC_Scheduler/
│
├── FreeRTOS/                    # FreeRTOS çekirdek dosyaları
│   ├── include/                 # Header dosyaları
│   ├── portable/                # Platform portları
│   │   └── ThirdParty/GCC/Posix/
│   └── source/                  # Kaynak kodları
│
├── src/                         # Proje kaynak kodları
│   ├── main.c                   # Ana program
│   ├── scheduler.c              # Görevlendirici implementasyonu
│   ├── scheduler.h              # Görevlendirici header
│   └── tasks.c                  # İşçi görev fonksiyonları
│
├── Makefile                     # Derleme dosyası
├── giris.txt                    # Görev listesi
└── README.md                    # Bu dosya
```

## 📝 giris.txt Formatı

Her satır bir görevi tanımlar:
```
<varış_zamanı>, <öncelik>, <görev_zamanı>
```

Örnek:
```
0, 1, 2     → 0. sn'de gelir, öncelik 1, 2 sn sürer
1, 0, 1     → 1. sn'de gelir, öncelik 0 (RT), 1 sn sürer
1, 3, 2     → 1. sn'de gelir, öncelik 3, 2 sn sürer
```

## 🔍 Algoritma Özeti

### Ana Döngü (Her Saniye)
1. **Yeni Görevleri Al**: `arrivalTime == globalTime` olan görevleri kuyruklara yerleştir
2. **Görev Seç**: En yüksek öncelikli kuyruğun başındaki görevi seç
3. **Çalıştır**: Görevi 1 saniye çalıştır (`remainingTime--`)
4. **Yönet**:
   - Bitti mi? → Sonlandır
   - RT görevi mi? → Devam et (kesintisiz)
   - Kullanıcı görevi mi? → Askıya al, öncelik düşür, yeni kuyruğa ekle
5. **Zamanı İlerlet**: `globalTime++`
6. **ZamanaSIMI Kontrolü**: 20 saniye bekleyen görevleri sonlandır

### Veri Yapıları

#### TaskInfo (Görev Bilgisi)
```c
- id              : Görev kimliği
- arrivalTime     : Geldiği zaman
- priority        : Öncelik (0: RT, 1-3+: Kullanıcı)
- burstTime       : Toplam süre
- remainingTime   : Kalan süre
- waitCounter     : Bekleme sayacı (zamanaSIMI için)
- hasStarted      : Daha önce başladı mı?
- isFinished      : Tamamlandı mı?
```

#### Queue (Kuyruk)
```c
- head  : İlk görev
- tail  : Son görev
- count : Görev sayısı
```

## 🎓 Öğrenme Hedefleri

Bu proje ile öğrenilecekler:
1. **FreeRTOS Görev Yönetimi**: Task oluşturma, askıya alma, silme
2. **Öncelikli Görevlendirme**: Farklı öncelik seviyelerinin etkisi
3. **Geri Beslemeli Kuyruk**: Dinamik öncelik değişimi
4. **Gerçek Zamanlı Sistemler**: Deterministik davranış, zamanaSIMI
5. **Veri Yapıları**: Bağlı liste, kuyruk (FIFO)

## 📚 Kaynaklar

- [FreeRTOS Resmi Dokümantasyonu](https://www.freertos.org/Documentation/RTOS_book.html)
- [FreeRTOS GitHub Repository](https://github.com/FreeRTOS/FreeRTOS-Kernel)
- [POSIX Port Dokümantasyonu](https://www.freertos.org/FreeRTOS-simulator-for-Linux.html)

## 🐛 Hata Ayıklama İpuçları

1. **Derleme Hatası**: `FreeRTOS/include` ve `FreeRTOS/portable/ThirdParty/GCC/Posix` yollarının doğru olduğundan emin olun
2. **Dosya Bulunamadı**: `giris.txt` dosyasının çalıştırma dizininde olduğundan emin olun
3. **Beklenmeyen Çıktı**: `globalTime` ve `waitCounter` değerlerini kontrol edin
4. **ZamanaSIMI Çalışmıyor**: Sadece kullanıcı kuyrukları (P1, P2, P3) için geçerlidir, RT için değil

## 📄 Lisans

Bu proje, İşletim Sistemleri dersi kapsamında eğitim amaçlı geliştirilmiştir.