#ifndef SWITCH_AUDIO_SWITCH_H_
#define SWITCH_AUDIO_SWITCH_H_

#include "sdk_errors.h"

typedef enum {
	OFF,
	LOW,
	HIGH
} audio_switch_position_t;

ret_code_t audio_switch_init(void);
audio_switch_position_t audio_switch_get_position(void);

#endif /* SWITCH_AUDIO_SWITCH_H_ */
