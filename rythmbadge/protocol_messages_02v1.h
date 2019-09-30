#ifndef __PROTOCOL_MESSAGES_02V1_H
#define __PROTOCOL_MESSAGES_02V1_H

#include <stdint.h>
#include "common_messages.h"

#define Request_status_request_tag 1
#define Request_start_microphone_request_tag 2
#define Request_stop_microphone_request_tag 3
#define Request_start_scan_request_tag 4
#define Request_stop_scan_request_tag 5
#define Request_start_imu_request_tag 6
#define Request_stop_imu_request_tag 7
#define Request_identify_request_tag 27
#define Request_restart_request_tag 29

#define Response_status_response_tag 1
#define Response_start_microphone_response_tag 2
#define Response_start_scan_response_tag 3
#define Response_start_accelerometer_response_tag 4

typedef struct {
	Timestamp timestamp;
	uint8_t has_badge_assignement;
	BadgeAssignement badge_assignement;
} StatusRequest;

typedef struct {
	Timestamp timestamp;
} StartMicrophoneRequest;

typedef struct {
} StopMicrophoneRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t timeout;
	uint16_t window;
	uint16_t interval;
	uint16_t duration;
	uint16_t period;
	uint8_t aggregation_type;
} StartScanRequest;

typedef struct {
} StopScanRequest;

typedef struct {
	Timestamp timestamp;
	uint16_t acc_fsr;
	uint16_t gyr_fsr;
	uint16_t datarate;
} StartImuRequest;

typedef struct {
} StopImuRequest;

typedef struct {
	uint16_t timeout;
} IdentifyRequest;

typedef struct {
} RestartRequest;

typedef struct __attribute__((__packed__)){
	uint8_t which_type;
	union {
		StatusRequest status_request;
		StartMicrophoneRequest start_microphone_request;
		StopMicrophoneRequest stop_microphone_request;
		StartScanRequest start_scan_request;
		StopScanRequest stop_scan_request;
		StartImuRequest start_accelerometer_request;
		StopImuRequest stop_accelerometer_request;
		IdentifyRequest identify_request;
		RestartRequest restart_request;
	} type;
} Request;

typedef struct {
	uint8_t clock_status;
	uint8_t microphone_status;
	uint8_t scan_status;
	uint8_t imu_status;
	Timestamp timestamp;
} StatusResponse;

typedef struct {
	Timestamp timestamp;
} StartMicrophoneResponse;

typedef struct {
	Timestamp timestamp;
} StartScanResponse;

typedef struct {
	Timestamp timestamp;
} StartImuResponse;

typedef struct {
	uint8_t which_type;
	union {
		StatusResponse status_response;
		StartMicrophoneResponse start_microphone_response;
		StartScanResponse start_scan_response;
		StartImuResponse start_imu_response;
	} type;
} Response;

#endif
