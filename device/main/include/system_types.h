#ifndef SYSTEM_TYPES_H
#define SYSTEM_TYPES_H

#include "../bool_t.h"

typedef struct {
    bool_t temperature;
    bool_t wifi;
    bool_t sd;
} anomaly_t;

typedef struct {
    int16_t temperature;
} telemetry_t;

typedef struct {
    telemetry_t telemetry;
    anomaly_t anomaly;
} system_t;

#endif