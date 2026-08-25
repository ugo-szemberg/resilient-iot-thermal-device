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
    anomaly_t anomaly;
    telemetry_t telemetry;
} system_t;

#endif