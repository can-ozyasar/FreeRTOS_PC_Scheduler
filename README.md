# FreeRTOS PC Scheduler

FreeRTOS zamanlayıcı mantığını PC ortamında incelemek için C/Makefile tabanlı çalışma.

## Bu Repo Ne İçin Var?
FreeRTOS zamanlayıcı davranışını ve görev mantığını PC ortamında gözlemlemek için oluşturuldu.

Bu README'nin amacı; repoya ilk kez gelen birinin projenin neden açıldığını, içinde ne bulunduğunu ve nereden başlaması gerektiğini hızlıca anlamasını sağlamaktır.

## İçerik ve Kapsam
Bu repoda öne çıkan içerikler şunlardır:
- Makefile ile derlenebilir proje
- Zamanlayıcı ve görev yönetimi deneyleri
- Gömülü sistem kavramlarını masaüstünde test etme amacı
- Makefile ile derleme/çalıştırma akışı

## Kimler İçin Faydalı?
C/C++ veya sistem programlama konularını uygulama üzerinden görmek isteyenler için uygundur.

## Kullanılan Teknolojiler
- C
- Make

## Kurulum
```bash
make
```

## Çalıştırma
```bash
make run
```

## Önemli Dosyalar
- `Makefile`

## Proje Yapısı
- `FreeRTOS` - 46 dosya
- `src` - 9 dosya
- `Makefile` - 1 dosya
- `algoritma.md` - 1 dosya
- `cikti.txt` - 1 dosya
- `freertos_sim` - 1 dosya
- `giris.txt` - 1 dosya

## Geliştirme Notları
- README içeriği, repodaki mevcut dosya yapısı ve proje açıklamasına göre düzenlenmiştir.
- Yeni modül, veri seti veya servis eklendiğinde kurulum/çalıştırma bölümlerini güncelleyin.

## Lisans
Bu repoda açık bir lisans dosyası yoksa tüm haklar varsayılan olarak proje sahibine aittir. Paylaşım veya kullanım koşulları için repo sahibine danışın.
