/* src/main.c */
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "scheduler.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Kullanım: %s giris.txt\n", argv[0]);
        return 1;
    }

    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif

    printf("=== FreeRTOS Simülasyonu ===\n");
    
    SchedulerInit(argv[1]);
    vTaskStartScheduler();
    
    return 1;
}