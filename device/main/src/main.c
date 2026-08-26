#include "../include/system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../bool_t.h"

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(500));
    
    system_init_watchdog();
    system_init_events();
    xTaskCreatePinnedToCore(system_start_wifi, "SYSTEM_START_WIFI", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(system_update_state, "SYSTEM_UPDATE_STATE", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(system_init_sd_card, "SYSTEM_INIT_SD_CARD", 4096, NULL, 3, NULL, 1);
}