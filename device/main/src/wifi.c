#include "../include/wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <stdio.h>

#define SSID "CUBESAT_WIFI"
#define PASSWORD "NANOSATELLITE"
#define UDP_SERVER_IP "192.168.4.255"
#define UDP_SERVER_PORT 3333
#define WIFI_CONNECTED_BIT BIT0
#define TAG_WIFI (const char*)("WIFI")

static EventGroupHandle_t s_wifi_event_group = NULL;

static int sock = -1;
static struct sockaddr_in addr;
static bool_t wifi_initialized = BOOL_FALSE;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG_WIFI, "AP START");
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                break;

            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG_WIFI, "DEVICE CONNECTED");
                break;

            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG_WIFI, "DEVICE DECONNECTED");
                break;

            default:
                break;
        }
    }
}

static void wifi_create_socket(void)
{
    addr.sin_addr.s_addr = inet_addr(UDP_SERVER_IP);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_SERVER_PORT);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(sock < 0)
    {
        ESP_LOGI(TAG_WIFI, "Error socket");
        vTaskDelete(NULL);
    }
}

void wifi_init(void)
{        
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,  &event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = SSID,
            .ssid_len = strlen(SSID),
            .password = PASSWORD,
            .channel = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = 3,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "WIFI ENABLED");

    wifi_create_socket();

    wifi_initialized = BOOL_TRUE;
}

void wifi_close(void)
{
    wifi_initialized = BOOL_FALSE;
    close(sock);
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());

    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, NULL);

    vEventGroupDelete(s_wifi_event_group);
    s_wifi_event_group = NULL;
    
    ESP_LOGI(TAG_WIFI, "WIFI DISABLED");
}

bool_t wifi_enabled(void)
{
    return wifi_initialized;
}

void wifi_send_datas(void* datas, int bytes)
{
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

    ssize_t error = sendto(sock, datas, bytes, 0, (struct sockaddr*) &addr, sizeof(addr));

    if(error < 0)
    {
        ESP_LOGI(TAG_WIFI, "FAILED TO SEND DATAS");
    }
    else
    {
        ESP_LOGI(TAG_WIFI, "SUCCEEDED TO SEND DATAS");
    }
}