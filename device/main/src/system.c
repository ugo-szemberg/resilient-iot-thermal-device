#include "../include/system.h"
#include "../include/temperature_sensor.h"
#include "../include/system_types.h"
#include "../include/wifi.h"
#include "../include/sd_card.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
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
#define EVENT_WIFI_INIT BIT0
#define EVENT_SD_INIT BIT1

typedef enum {
    INITIAL,
    ACQUISITION,
    TRANSMISSION,
    NOMINAL,
    CRITICAL,
    SLEEPING
} state_t;

static state_t system_state = INITIAL;
static uint32_t time_to_sleep_uS = TIME_SLEEP_S_NOMINAL * uS_TO_S_FACTOR;
static RTC_DATA_ATTR uint32_t lost_packet_count = 0U;
static EventGroupHandle_t system_events;

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

void system_init_events(void)
{
    system_events = xEventGroupCreate();
}

void system_init_datas(system_t* datas)
{
    datas->telemetry.temperature = 0;
    datas->anomaly.temperature = BOOL_FALSE;
    datas->anomaly.wifi = BOOL_FALSE;
    datas->anomaly.sd = BOOL_FALSE;
}

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
    if(wifi_enabled() == BOOL_FALSE)
    {
        wifi_init();
    }
    xEventGroupSetBits(system_events, EVENT_WIFI_INIT);
    vTaskDelete(NULL);
}

void system_init_sd_card(void* arg)
{
    if(sd_card_enabled() == BOOL_FALSE)
    {
        sd_card_init();
    }
    xEventGroupSetBits(system_events, EVENT_SD_INIT);
    vTaskDelete(NULL);
}

void system_serialize_datas(system_t* datas, uint8_t* buffer)
{
    buffer[0] = (uint8_t)datas->anomaly.temperature;
    buffer[1] = (uint8_t)datas->anomaly.wifi;
    buffer[2] = (uint8_t)datas->anomaly.sd;

    buffer[3] = (uint8_t)(datas->telemetry.temperature >> 8);
    buffer[4] = (uint8_t)(datas->telemetry.temperature & 0xFF);
    buffer[5] = (uint8_t)0U; //padding
}

static void system_handle_initial_state(void)
{
    ESP_LOGI(TAG_SYSTEM, "INITIAL");
    temperature_sensor_init();
}

static void system_handle_acquisition_state(void)
{
    ESP_LOGI(TAG_SYSTEM, "ACQUISITION");

    system_init_datas(&system_datas);

    system_datas.telemetry.temperature = temperature_sensor_get_temperature();
    
    if(temperature_sensor_has_anomaly(system_datas.telemetry.temperature) == BOOL_TRUE)
    {
        system_datas.anomaly.temperature = BOOL_TRUE;
        ESP_LOGI(TAG_TEMPERATURE_SENSOR, "ANOMALY");
    }

    if(sd_card_has_anomaly() == BOOL_TRUE)
    {
        system_datas.anomaly.sd = BOOL_TRUE;
        ESP_LOGI(TAG_SD_CARD, "ANOMALY");
    }
}

static void system_handle_transmission_state(void)
{
    EventBits_t bits = xEventGroupWaitBits(system_events, EVENT_WIFI_INIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(500));
    
    if(bits & EVENT_WIFI_INIT)
    {
        if(lost_packet_count > 0 && sd_card_enabled() == BOOL_TRUE)
        {
            uint8_t buffer[sizeof(system_t) * lost_packet_count] = {0};
            sd_card_read(buffer, sizeof(buffer));

            for(uint32_t i = 0U; i < lost_packet_count; ++i)
            {
                wifi_send_datas((void*)&buffer[i * sizeof(system_t)], (int)sizeof(system_t));
            }

            sd_card_clean();
            lost_packet_count = 0U;
        }

        uint8_t buffer[sizeof(system_t)] = {0};
        system_serialize_datas(&system_datas, buffer);
        wifi_send_datas((void*)&buffer, (int)sizeof(buffer));
    }
    else
    {
        system_datas.anomaly.wifi = BOOL_TRUE;

        if(sd_card_enabled() == BOOL_TRUE)
        {
            if(bits & EVENT_SD_INIT)
            {
                uint8_t buffer[sizeof(system_t)] = {0};
                system_serialize_datas(&system_datas, buffer);
                sd_card_write((void*)&buffer, sizeof(buffer));
                lost_packet_count += 1U;
            }
        }
    }

    if(system_datas.anomaly.temperature == BOOL_TRUE)
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

    time_to_sleep_uS = TIME_SLEEP_S_NOMINAL * uS_TO_S_FACTOR;
}

static void system_handle_critical_state(void)
{
    ESP_LOGI(TAG_SYSTEM, "CRITICAL");
    system_state = ACQUISITION;
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
            system_state = ACQUISITION;
            break;
        case ACQUISITION:
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