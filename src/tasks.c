/* src/main.c */
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

/**
 * FreeRTOS Görevlendirici Simülasyonu - Ana Program
 * 
 * Bu program, 4 seviyeli öncelikli görevlendirici simülasyonunu çalıştırır:
 * - Öncelik 0: Gerçek Zamanlı görevler (FCFS, kesintisiz)
 * - Öncelik 1-3+: Kullanıcı görevleri (3 seviyeli geri beslemeli kuyruk)
 * 
 * Kullanım: ./freertos_sim giris.txt
 * 
 * @param argc Argüman sayısı
 * @param argv Argüman dizisi (argv[1] = görev listesi dosyası)
 */
int main(int argc, char *argv[]) {
    // Komut satırı argümanı kontrolü
    if (argc < 2) {
        printf("HATA: Görev listesi dosyası belirtilmedi!\n");
        printf("Kullanım: %s giris.txt\n", argv[0]);
        printf("\nÖrnek: ./freertos_sim giris.txt\n");
        return 1;
    }

    // Terminal ekranını temizle (platform bağımsız)
    #ifdef _WIN32
        system("cls");  // Windows
    #else
        system("clear"); // Linux/macOS
    #endif

    printf("========================================\n");
    printf("  FreeRTOS Görevlendirici Simülasyonu  \n");
    printf("  4 Seviyeli Öncelikli Görevlendirici  \n");
    printf("========================================\n\n");

    // Görevlendiriciyi başlat
    SchedulerInit(argv[1]);
    
    // FreeRTOS görev zamanlayıcısını başlat
    // Bu fonksiyon asla geri dönmez, simülasyon exit() ile sonlanır
    vTaskStartScheduler();
    
    // Buraya asla ulaşılmamalı
    printf("HATA: Görevlendirici başlatılamadı!\n");
    return 1;
}