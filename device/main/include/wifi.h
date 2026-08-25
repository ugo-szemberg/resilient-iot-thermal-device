#ifndef WIFI_H
#define WIFI_H

#include "../bool_t.h"

void wifi_init(void);
void wifi_close(void);
bool_t wifi_enabled(void);
void wifi_send_datas(void* datas, int bytes);

#endif