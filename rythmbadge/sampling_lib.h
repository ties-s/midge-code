/**@file
 * @details
 *
 */

#ifndef __SAMPLING_LIB_H
#define __SAMPLING_LIB_H

#include "stdint.h"
#include "sdk_errors.h"	// Needed for the definition of ret_code_t and the error-codes
#include "nrf_pdm.h"


typedef enum {
	SAMPLING_IMU		 				= (1 << 0),
	SAMPLING_MICROPHONE 				= (1 << 1),
	SAMPLING_SCAN 						= (1 << 2),
} sampling_configuration_t;



/**@brief This function initializes all the data-sources and -structures needed to sample the different data-sources.
 *
 * @details It initalizes all the different data-source modules (accel (if available), battery, scanner, microphone.
 *			For the data-sampling chunk-fifos are used to provide an efficient and clean way to perform fast data-sampling and data-exchange.
 *			For the data-streaming circular-fifos are used.
 *			Furthermore it initializes the timeout-mechanism for the data-sources.
 *
 * @retval 	NRF_SUCCESS		If everything was initialized correctly.
 * @retval	Otherwise an error code is provided.
 *
 * @note app-timer has to be initialized before!
 * @note app-scheduler has to be initialized before!
 * @note timeout_lib has to be initialized before!
 * @note systick_lib has to be initialized before!
 * @note advertiser_lib has to be initialized before!
 */
ret_code_t sampling_init(void);


/**@brief Function to retrieve the current sampling-/streaming-configuration (So which data-sources are currently sampling).
 *
 * @retval 	The current sampling-configuration.
 */
sampling_configuration_t sampling_get_sampling_configuration(void);


/**@brief Function to start the accelerometer
 *
 * @param[in]	acc_fsr		[2, 4, 8, 16]
 * @param[in]	gyr_fsr		[]
 * @param[in]	datrate		[50,100,200]
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_start_imu(uint16_t acc_fsr, uint16_t gyr_fsr, uint16_t datarate);

/**@brief Function to stop the imu
 *
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_stop_imu(void);




/**@brief Function to start the microphone data recording or streaming.
 *
 * @param[in]	mode 					PDM mode, 0=stereo, 1=mono
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_start_microphone(nrf_pdm_mode_t mode);

/**@brief Function to stop the microphone data recording or streaming.
 *
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_stop_microphone(void);



/**@brief Function to start the scan data recording or streaming.
 *
 * @param[in]	interval_ms 				The interval of the scan.
 * @param[in]	window_ms 					The window of the scan.
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_start_scan(uint16_t interval_ms, uint16_t window_ms);

/**@brief Function to stop the scan data recording or streaming.
 *
 * @param[in]	streaming					Flag if streaming or data acquisition should be stopped [0 == data-acquisistion, 1 == streaming].
 *
 * @retval		NRF_SUCCESS 	If everything was ok.
 * @retval						Otherwise an error code is returned.
 */
ret_code_t sampling_stop_scan(void);



#endif
