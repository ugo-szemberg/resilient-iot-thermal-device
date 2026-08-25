#include "../include/devices.h"
#include "../include/temperature_sensor.h"
#include "../include/sd_card.h"
#include "esp_log.h"

void devices_init(void)
{
    temperature_sensor_init();
}

void devices_get_datas(telemetry_t* telemetry)
{
    telemetry->temperature = temperature_sensor_get_temperature();
}

bool_t devices_get_anomalies(system_t* datas)
{
    bool_t anomaly_detected = BOOL_FALSE;

    if(temperature_sensor_has_anomaly(datas->telemetry.temperature) == BOOL_TRUE)
    {
        datas->anomaly.temperature = BOOL_TRUE;
        anomaly_detected = BOOL_TRUE;
        ESP_LOGI(TAG_TEMPERATURE_SENSOR, "ANOMALY");
    }

    if(sd_card_has_anomaly() == BOOL_TRUE)
    {
        datas->anomaly.sd = BOOL_TRUE;
        ESP_LOGI(TAG_SD_CARD, "ANOMALY");
    }

    return anomaly_detected;
}