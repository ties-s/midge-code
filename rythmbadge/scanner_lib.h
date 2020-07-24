#ifndef __SCANNER_LIB_H
#define __SCANNER_LIB_H

#include "stdint.h"
#include "sdk_errors.h"
#include "protocol_messages.h"

#define SCANNER_MINIMUM_RSSI	(-100)

typedef struct {
	uint64_t		timestamp;
	BadgeAssignment	badge_assignment;
	int8_t 			rssi;
} scanner_scan_report_t;
// 12 bytes

#define SCANNER_BUFFER_LENGTH 128 // to get 3*512B (block size)
extern scanner_scan_report_t scanner_scan_buffer[2][SCANNER_BUFFER_LENGTH];


/**@brief Function to initialize the scanner.
 *
 * @note ble_init() has to be called before.
 */
void 		scanner_init(void);


/**@brief Function to start a scan-operation.
 * 
 * @param[in] scan_interval_ms		The scan interval in milliseconds.
 * @param[in] scan_window_ms		The scan window in milliseconds.
 *
 * @note		The input-parameters of this function has to be chosen in a way that advertising is still possible.
 */
ret_code_t 	scanner_start_scanning(uint16_t scan_interval_ms, uint16_t scan_window_ms);


/**@brief	Function for stopping any ongoing scan-operation.
 */
void 		scanner_stop_scanning(void);

#endif
