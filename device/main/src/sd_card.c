#include "../include/sd_card.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include <stdio.h>

#define PIN_NUM_MISO GPIO_NUM_13
#define PIN_NUM_MOSI GPIO_NUM_11
#define PIN_NUM_SCK GPIO_NUM_12
#define PIN_NUM_CS GPIO_NUM_10
#define MAX_TRANSFER_SIZE 512
#define MAX_FILES 5

static sdmmc_card_t* card = NULL;
static sdmmc_host_t host = SDSPI_HOST_DEFAULT();
static bool_t sd_card_initialized = BOOL_FALSE;
static bool_t sd_card_anomaly = BOOL_FALSE;

void sd_card_init(void)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = MAX_TRANSFER_SIZE,
    };

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_config, SPI_DMA_CH_AUTO);
    if(ret != ESP_OK)
    {
        ESP_LOGI(TAG_SD_CARD, "FAILED TO INITIALIZE");
        sd_card_anomaly = BOOL_TRUE;
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = MAX_FILES,
    };

    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG_SD_CARD, "FAILED TO INITIALIZE");
        spi_bus_free(host.slot);
        sd_card_anomaly = BOOL_TRUE;
        return;
    }
    else
    {
        ESP_LOGI(TAG_SD_CARD, "SUCCEEDED TO INITIALIZE");
        sdmmc_card_print_info(stdout, card);
        sd_card_initialized = BOOL_TRUE;
    }
}

void sd_card_write(void* datas, size_t bytes)
{
    FILE* file;
    file = fopen("/sdcard/logs.bin", "ab");
    if(file == NULL)
    {
        ESP_LOGE(TAG_SD_CARD, "FAILED TO OPEN FILE");
        return;
    }

    if(fwrite(datas, bytes, 1, file) == 1)
    {
        ESP_LOGI(TAG_SD_CARD, "SUCCEEDED TO WRITE DATAS");
    }
    else
    {
        ESP_LOGE(TAG_SD_CARD, "FAILED TO WRITE DATAS");
    }
    
    fclose(file);
}

bool_t sd_card_has_anomaly()
{
    return sd_card_anomaly;
}

bool_t sd_card_enabled(void)
{
    return sd_card_initialized;
}

void sd_card_close(void)
{
    if (card != NULL)
    {
        esp_vfs_fat_sdcard_unmount("/sdcard", card);
        card = NULL;
    }

    sdspi_host_deinit();
    spi_bus_free(host.slot);
    sd_card_initialized = BOOL_FALSE;
    ESP_LOGI(TAG_SD_CARD, "SD CLOSED");
}