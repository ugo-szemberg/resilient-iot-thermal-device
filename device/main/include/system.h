#ifndef SYSTEM_H
#define SYSTEM_H

#include "../bool_t.h"

void system_init_events(void);
void system_init_watchdog(void);
void system_start_wifi(void* arg);
void system_init_sd_card(void* arg);
void system_update_state(void* arg);

#endif