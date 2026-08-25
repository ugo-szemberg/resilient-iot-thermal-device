#include "../include/temperature_sensor.h"
#include "esp_adc/adc_oneshot.h"

#define TEMPERATURE_SENSOR_MAX (int16_t)50
#define VOLTAGE (uint16_t)3300
#define ADC_RESOLUTION (uint16_t)4095
#define OFFSET (uint16_t)500
#define CONVERSION (uint16_t)10
#define ADC_UNIT ADC_UNIT_1
#define CHANNEL ADC_CHANNEL_0

static adc_oneshot_unit_handle_t adc_handle;

void temperature_sensor_init(void)
{
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT,
    };
    adc_oneshot_new_unit(&adc_config, &adc_handle);

    adc_oneshot_chan_cfg_t adc_channel = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    adc_oneshot_config_channel(adc_handle, CHANNEL, &adc_channel);
}

int16_t temperature_sensor_get_temperature(void)
{
    int adc_value;
    adc_oneshot_read(adc_handle, CHANNEL, &adc_value);
    uint16_t mV = ((uint16_t)adc_value * VOLTAGE) / ADC_RESOLUTION;
    int16_t temperature_value = (int16_t)((mV - OFFSET) / CONVERSION);
    return temperature_value;
}

bool_t temperature_sensor_has_anomaly(int16_t temperature)
{
    if(temperature > TEMPERATURE_SENSOR_MAX)
    {
        return BOOL_TRUE;
    }
    return BOOL_FALSE;
}