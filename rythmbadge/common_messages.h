#ifndef __COMMON_MESSAGES_H
#define __COMMON_MESSAGES_H

#include <stdint.h>

typedef struct __attribute__((__packed__)) {
	uint32_t seconds;
	uint16_t ms;
} Timestamp;

typedef struct {
	uint16_t ID;
	uint8_t group;
} BadgeAssignement;

typedef struct {
	uint16_t ID;
	int8_t rssi;
} ScanDevice;

typedef struct {
	ScanDevice scan_device;
	uint8_t count;
} ScanResultData;

#endif
