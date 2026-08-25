#ifndef DEVICES_H
#define DEVICES_H

#include "../bool_t.h"
#include "system_types.h"

void devices_init(void);
void devices_get_datas(telemetry_t* telemetry);
bool_t devices_get_anomalies(system_t* datas);

#endif