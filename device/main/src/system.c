#include "../include/system.h"
#include "../include/devices.h"
#include "../include/system_types.h"
#include "../include/wifi.h"
#include "../include/sd_card.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_sleep.h"
#include <stdio.h>
#include <inttypes.h>
#include <stddef.h>

#define uS_TO_S_FACTOR (uint32_t)1000000ULL
#define TIME_SLEEP_S_NOMINAL (uint32_t)10U
#define TAG_SYSTEM (const char*)("SYSTEM")

typedef enum {
    INITIAL,
    AQUISITION,
    TRANSMISSION,
    NOMINAL,
    CRITICAL,
    SLEEPING
} state_t;

static state_t system_state = INITIAL;
static uint32_t time_to_sleep_uS = TIME_SLEEP_S_NOMINAL * uS_TO_S_FACTOR;
static SemaphoreHandle_t semaphore_wifi;
static SemaphoreHandle_t semaphore_sd_card;
static bool_t anomaly_detected = BOOL_FALSE;

static system_t system_datas =
{
    .telemetry =
    {
        .temperature = 0
    },
    .anomaly = 
    {
        .temperature = BOOL_FALSE,
        .wifi = BOOL_FALSE,
        .sd = BOOL_FALSE
    }
};

void system_init_watchdog(void)
{
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 1000,
        .idle_core_mask = (1 << 0) | (1 << 1),
        .trigger_panic = true,
    };
    esp_task_wdt_reconfigure(&twdt_config);
}

void system_start_wifi(void* arg)
{
    semaphore_wifi = xSemaphoreCreateBinary();
    if(wifi_enabled() == BOOL_FALSE)
    {
        wifi_init();
        xSemaphoreGive(semaphore_wifi);
    }
    vTaskDelete(NULL);
}

void system_init_sd_card(void* arg)
{
    semaphore_sd_card = xSemaphoreCreateBinary();
    if(sd_card_enabled() == BOOL_FALSE)
    {
        sd_card_init();
        xSemaphoreGive(semaphore_sd_card);
    }
    vTaskDelete(NULL);
}

void system_serialize_datas(system_t* datas, uint8_t* buffer)
{
    buffer[0] = (uint8_t)datas->anomaly.temperature;
    buffer[1] = (uint8_t)datas->anomaly.wifi;
    buffer[2] = (uint8_t)datas->anomaly.sd;

    buffer[3] = (uint8_t)(datas->telemetry.temperature >> 8);
    buffer[4] = (uint8_t)(datas->telemetry.temperature & 0xFF);
}

static void system_handle_initial_state(void)
{
    ESP_LOGI(TAG_SYSTEM, "INITIAL");
    devices_init();
}

static void system_handle_acquisition_state(void)
{
    ESP_LOGI(TAG_SYSTEM, "AQUISITION");

    devices_get_datas(&system_datas.telemetry);
    anomaly_detected = devices_get_anomalies(&system_datas);
}

static void system_handle_transmission_state(void)
{
    if(xSemaphoreTake(semaphore_wifi, pdMS_TO_TICKS(500)) == pdTRUE)
    {
        uint8_t buffer[sizeof(system_t)] = {0};
        system_serialize_datas(&system_datas, buffer);
        wifi_send_datas((void*)&buffer, (int)sizeof(buffer));
    }
    else
    {
        system_datas.anomaly.wifi = BOOL_TRUE; 
        anomaly_detected = BOOL_TRUE;
    }

    if(anomaly_detected == BOOL_TRUE)
    {
        system_state = CRITICAL;
    }
    else
    {
        system_state = NOMINAL;
    }
}

static void system_handle_nominal_state(void)
{
    ESP_LOGI(TAG_SYSTEM, "NOMINAL");

    if(sd_card_enabled() == BOOL_TRUE)
    {
        if(xSemaphoreTake(semaphore_sd_card, pdMS_TO_TICKS(500) == pdTRUE))
        {
            uint8_t buffer[sizeof(system_t)] = {0};
            system_serialize_datas(&system_datas, buffer);
            sd_card_write((void*)&buffer, sizeof(buffer));
        }
    }

    time_to_sleep_uS = TIME_SLEEP_S_NOMINAL * uS_TO_S_FACTOR;
}

static void system_handle_critical_state(void)
{
    ESP_LOGI(TAG_SYSTEM, "CRITICAL");
    system_state = AQUISITION;
}

static void system_handle_sleeping_state(void)
{
    ESP_LOGI(TAG_SYSTEM, "SLEEPING");
    if(wifi_enabled() == BOOL_TRUE)
    {
        wifi_close();
    }
    if(sd_card_enabled() == BOOL_TRUE)
    {
        sd_card_close();
    }
    esp_sleep_enable_timer_wakeup(time_to_sleep_uS);
    esp_deep_sleep_start();
}

void system_update_state(void* arg)
{
    esp_task_wdt_add(NULL);

    while(1)
    {
        switch(system_state)
        {
        case INITIAL:
            system_handle_initial_state();
            system_state = AQUISITION;
            break;
        case AQUISITION:
            system_handle_acquisition_state();
            system_state = TRANSMISSION;
            break;
        case TRANSMISSION:
            system_handle_transmission_state();
            break;
        case NOMINAL:
            system_handle_nominal_state();
            system_state = SLEEPING;
            break;
        case CRITICAL:
            system_handle_critical_state();
            break;
        case SLEEPING:
            system_handle_sleeping_state();
            break;
        default:
            break;
        }

        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}