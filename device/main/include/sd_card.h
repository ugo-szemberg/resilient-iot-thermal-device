#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdint.h>
#include <stddef.h>
#include "../bool_t.h"

#define TAG_SD_CARD (const char*)("SD_CARD")

void sd_card_init(void);
void sd_card_write(void* datas, size_t bytes);
bool_t sd_card_has_anomaly(void);
bool_t sd_card_enabled(void);
void sd_card_close(void);

#endif