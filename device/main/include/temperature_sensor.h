#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <stdint.h>
#include "../bool_t.h"

#define TAG_TEMPERATURE_SENSOR (const char*)("TEMPERATURE_SENSOR")

void temperature_sensor_init(void);
int16_t temperature_sensor_get_temperature(void);
bool_t temperature_sensor_has_anomaly(int16_t temperature);

#endif