#ifndef __ADVERTISER_LIB_H
#define __ADVERTISER_LIB_H

#include "stdint.h"
#include "sdk_errors.h"
#include "protocol_messages.h"	// For BadgeAssignment


#define ADVERTISING_DEVICE_NAME			"HDBDG"     /**< Name of device. Will be included in the advertising data. */ 
#define ADVERTISING_INTERVAL_MS		  	200   		/**< The advertising interval. */
#define ADVERTISING_TIMEOUT_SECONDS    	0       /**< The advertising timeout interval. */ //that's in 10ms units so 60ms

#define ADVERTISING_RESET_ID			0xFFFF
#define ADVERTISING_RESET_GROUP			0xFF

#define ADVERTISING_DEFAULT_GROUP		1


/**@brief Function to initialize the advertiser.
 *
 * @note ble_init() has to be called before.
 * @note storer_init() has to be called before.
 */
void advertiser_init(void);

/**@brief	Function to start the advertising process with the parameters: ADVERTISING_DEVICE_NAME, ADVERTISING_INTERVAL_MS, ADVERTISING_TIMEOUT_SECONDS.
 *
 * @retval	NRF_SUCCESS On success, else an error code indicating reason for failure.
 */
ret_code_t advertiser_start_advertising(void);


/**@brief Function to set the badge assignement (ID + group) of the advertising-packet.
 *
 * @param[in]	voltage				The battery voltage to set.
 */
void advertiser_set_battery_percentage(uint8_t battery_percentage);

/**@brief Function to set the badge assignement (ID + group) of the advertising-packet.
 *
 * @details	This function also tries to store the badge assignement to the filesystem, if necessary.
 *			If the badge assignement is ADVERTISING_RESET_ID, ADVERTISING_RESET_GROUP the 
 *			default ID and group will be taken, and the badge-assignement will be deleted from the filesystem.
 *
 * @param[in]	badge_assignement		The badge assignement to set.
 *
 * @retval		NRF_SUCCESS			If everything was successful.
 * @retval		NRF_ERROR_INTERNAL	If an internal error occured (e.g. busy) --> retry it.
 * @retval		Another error code	Probably of a false configuration of the filesystem partition for the badge assignement.
 */
ret_code_t advertiser_set_badge_assignement(BadgeAssignment badge_assignement);


/**@brief Function to set the is_clock_synced-status flag of the advertising-packet.
 *
 * @param[in]	is_clock_synced						Flag if clock is synchronized.
 */
void advertiser_set_status_flag_is_clock_synced(uint8_t is_clock_synced);

/**@brief Function to set the microphone_enabled-status flag of the advertising-packet.
 *
 * @param[in]	microphone_enabled					Flag if microphone is enabled.
 */
void advertiser_set_status_flag_microphone_enabled(uint8_t microphone_enabled);

/**@brief Function to set the scan_enabled-status flag of the advertising-packet.
 *
 * @param[in]	scan_enabled						Flag if scanner is enabled.
 */
void advertiser_set_status_flag_scan_enabled(uint8_t scan_enabled);

/**@brief Function to set the imu_enabled-status flag of the advertising-packet.
 *
 * @param[in]	imu_enabled			Flag if imu is enabled.
 */
void advertiser_set_status_flag_imu_enabled(uint8_t imu_enabled);




/**@brief Function to retrieve the own badge-assignment.
 *
 * @param[out]	badge_assignement	Pointer where to store the badge_assignement.
 */
void advertiser_get_badge_assignement(BadgeAssignment* badge_assignement);


/**@brief Function to retrieve the badge assignement from a custom_advdata-packet.
 *
 * @param[out]	badge_assignement	Pointer where to store the badge_assignement.
 * @param[in]	custom_advdata		Pointer to custom_advdata.
 */
void advertiser_get_badge_assignement_from_advdata(BadgeAssignment* badge_assignement, void* custom_advdata);


/**@brief Function to retrieve the length of the manufacture-data.
 *
 * @retval Length of the manufacture data.
 */
uint8_t advertiser_get_manuf_data_len(void);

/**@brief Function to retrieve the is_clock_synced-status flag of the advertising-packet.
 *
 * @retval 	0			If clock is unsynced.
 * @retval 	1			If clock is synced.
 */
uint8_t advertiser_get_status_flag_is_clock_synced(void);

/**@brief Function to retrieve the microphone_enabled-status flag of the advertising-packet.
 *
 * @retval 	0			If microphone is not enabled.
 * @retval 	1			If microphone is enabled.
 */
uint8_t advertiser_get_status_flag_microphone_enabled(void);

/**@brief Function to retrieve the scan_enabled-status flag of the advertising-packet.
 *
 * @retval 	0			If scanner is not enabled.
 * @retval 	1			If scanner is enabled.
 */
uint8_t advertiser_get_status_flag_scan_enabled(void);

/**@brief Function to retrieve the imu-status flag of the advertising-packet.
 *
 * @retval 	0			If imu is not enabled.
 * @retval 	1			If imu is enabled.
 */
uint8_t advertiser_get_status_flag_imu_enabled(void);



#endif
