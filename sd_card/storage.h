#ifndef SD_CARD_STORAGE_H_
#define SD_CARD_STORAGE_H_

#include <stdbool.h>
#include <stdint.h>

#include "ff.h"

uint32_t storage_init(void);

void sd_close(void * p_event_data, uint16_t event_size);
void sd_write(void * p_event_data, uint16_t event_size);

#endif /* SD_CARD_STORAGE_H_ */
