/* src/main.c */
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

int main(int argc, char *argv[]) {
    /* Argüman kontrolü */
    if (argc < 2) {
        printf("Kullanım: ./freertos_sim <giris_dosyasi>\n");
        printf("Örnek:    ./freertos_sim giris.txt\n");
        return 1;
    }
    
    /* Ekranı temizle */
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
    
    /* Scheduler'ı başlat */
    SchedulerInit(argv[1]);
    
    /* FreeRTOS scheduler'ı çalıştır */
    vTaskStartScheduler();
    
    /* Buraya asla ulaşılmamalı */
    printf("Hata: vTaskStartScheduler geri döndü!\n");
    
    return 0;
}