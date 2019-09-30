#ifndef SD_CARD_STORAGE_H_
#define SD_CARD_STORAGE_H_

#include <stdbool.h>
#include <stdint.h>

#include "ff.h"

typedef enum{
	IMU,
	AUDIO
} data_source_t;

typedef enum{
	ACCEL,
	ACCEL_RAW,
	GYRO,
	GYRO_RAW,
	MAG,
	MAG_RAW,
	ROTATION_VECTOR,
	GAME_ROTATION_VECTOR,
	GEOMAGNETIC_ROTATION_VECTOR,
	MAX_IMU_SOURCES
} imu_source_t;

typedef struct{
	data_source_t data_source;
	union{
		uint8_t audio_buffer_num;
		imu_source_t imu_source;
	};
} data_source_info_t;

uint32_t storage_init(void);

void sd_close(void * p_event_data, uint16_t event_size);
void sd_write(void * p_event_data, uint16_t event_size);

uint32_t storage_init_folder(uint32_t sync_time_seconds);
uint32_t storage_open_file(data_source_t source);
uint32_t storage_close_file(data_source_t source);



#endif /* SD_CARD_STORAGE_H_ */
