#ifndef SYSTEM_TYPES_H
#define SYSTEM_TYPES_H

#include "../bool_t.h"

typedef struct {
    bool_t battery;
    bool_t solar_panels;
    bool_t temperature;
    bool_t wifi;
    bool_t sd;
} anomaly_t;

typedef struct {
    uint8_t battery_percent;
    uint16_t solar_production;
    int16_t temperature;
} telemetry_t;

typedef struct {
    telemetry_t telemetry;
    anomaly_t anomaly;
} system_t;

#endif