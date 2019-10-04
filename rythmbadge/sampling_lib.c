#include "sampling_lib.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "timeout_lib.h"

#include "scanner_lib.h"
#include "advertiser_lib.h"
#include "systick_lib.h"

#include "ICM20948_driver_interface.h"
#include "drv_audio_pdm.h"
#include "nrf_drv_pdm.h"
#include "storage.h"
#include "saadc.h"
#include "audio_switch.h"

//#define ABS(x) (((x) >= 0)? (x) : -(x))

#define AGGREGATE_SCAN_SAMPLE_MAX(sample, aggregated) 	((sample) > (aggregated) ? (sample) : (aggregated))
#define PROCESS_SCAN_SAMPLE_MAX(aggregated, count) 		(aggregated)

#define AGGREGATE_SCAN_SAMPLE_MEAN(sample, aggregated) 	((aggregated) + (sample))
#define PROCESS_SCAN_SAMPLE_MEAN(aggregated, count) 	((aggregated)/(count))


static sampling_configuration_t sampling_configuration;

//typedef struct {
//	uint16_t scan_period_seconds;
//	uint16_t scan_interval_ms;
//	uint16_t scan_window_ms;
//	uint16_t scan_duration_seconds;
//	uint8_t scan_group_filter;
//	uint8_t	scan_aggregation_type;	/**< The type of the aggregation: [SCAN_CHUNK_AGGREGATE_TYPE_MAX] == MAX, [SCAN_CHUNK_AGGREGATE_TYPE_MEAN] == MEAN */
//} sampling_scan_parameters_t;
//static sampling_scan_parameters_t sampling_scan_parameters;
/**< The scan:period is directly setted by starting the timer */

//APP_TIMER_DEF(sampling_scan_timer);
//void sampling_scan_callback(void* p_context); /**< Starts a scanning cycle */
//void sampling_on_scan_timeout_callback(void);
//void sampling_on_scan_report_callback(scanner_scan_report_t* scanner_scan_report);


ret_code_t sampling_init(void)
{
	ret_code_t ret = NRF_SUCCESS;
	(void) ret;
	
	sampling_configuration = (sampling_configuration_t) 0;
	
	/********************* IMU ***************************/
	ret = icm20948_init(); // Has to be run before audio switch because of gpiote_init
	if(ret != NRF_SUCCESS) return ret;	
	
	/********************* MICROPHONE ***********************************/
	drv_audio_init();
	
	/********************* SCAN ***************************************/
//	scanner_init();
//
//	// Set timeout and report callback function
//	scanner_set_on_scan_timeout_callback(sampling_on_scan_timeout_callback);
//	scanner_set_on_scan_report_callback(sampling_on_scan_report_callback);
//
//	// create a timer for scans
//	ret = app_timer_create(&sampling_scan_timer, APP_TIMER_MODE_REPEATED, sampling_scan_callback);
//	if(ret != NRF_SUCCESS) return ret;

	/** Battery **/
	saadc_init();

	/** Audio switch **/
	ret = audio_switch_init();
	if(ret != NRF_SUCCESS) return ret;

	return NRF_SUCCESS;
}

sampling_configuration_t sampling_get_sampling_configuration(void)
{
	return sampling_configuration;	
}



ret_code_t sampling_start_imu(uint16_t acc_fsr, uint16_t gyr_fsr, uint16_t datarate)
{
	ret_code_t ret = NRF_SUCCESS;
	
	// accel 2,4,6,8,16 g
	// gyro 250, 500, 1000, 2000 dps
	ret = icm20948_set_fsr((uint32_t)acc_fsr, (uint32_t)gyr_fsr);
	if(ret != NRF_SUCCESS) return ret;

	// TODO: datarates accepted? = do a check and set acceptable ones only - or in python, less work for me
	ret = icm20948_set_datarate(datarate);
	if(ret != NRF_SUCCESS) return ret;	

	ret = storage_open_file(IMU);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = icm20948_enable_sensors();
	if(ret != NRF_SUCCESS) return ret;
	
	sampling_configuration = (sampling_configuration_t) (sampling_configuration | SAMPLING_IMU);
	advertiser_set_status_flag_imu_enabled(1);
	
	return ret;
}

ret_code_t sampling_stop_imu(void)
{
	ret_code_t ret = NRF_SUCCESS;

	ret = icm20948_disable_sensors();
	if(ret != NRF_SUCCESS) return ret;

	ret = storage_close_file(IMU);
	if(ret != NRF_SUCCESS) return ret;

	sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(SAMPLING_IMU));
	advertiser_set_status_flag_imu_enabled(0);
	
	return ret;
}


/************************** MICROPHONE ****************************/
ret_code_t sampling_start_microphone(void)
{
	// TODO: 3 different modes - or just read the audio switch
	ret_code_t ret = NRF_SUCCESS;

	ret = storage_open_file(AUDIO);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = nrf_drv_pdm_start();
	if(ret != NRF_SUCCESS) return ret;
			
	sampling_configuration = (sampling_configuration_t) (sampling_configuration | SAMPLING_MICROPHONE);
	advertiser_set_status_flag_microphone_enabled(1);
			
	return ret;
}

ret_code_t sampling_stop_microphone(void)
{
	ret_code_t ret = NRF_SUCCESS;

	ret = nrf_drv_pdm_stop();
	if(ret != NRF_SUCCESS) return ret;

	ret = storage_close_file(AUDIO);
	if(ret != NRF_SUCCESS) return ret;

	sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(SAMPLING_MICROPHONE));
	advertiser_set_status_flag_microphone_enabled(0);

	return ret;
}

/************************** SCAN *********************************/
/*
ret_code_t sampling_start_scan(uint32_t timeout_ms, uint16_t period_seconds, uint16_t interval_ms, uint16_t window_ms, uint16_t duration_seconds, uint8_t group_filter, uint8_t aggregation_type, uint8_t streaming) {
	ret_code_t ret = NRF_SUCCESS;
	
	uint8_t parameters_changed_sampling = 0;
	
	if((timeout_ms != sampling_scan_parameters.scan_timeout_ms) && !streaming) 
		parameters_changed_sampling = 1;

	if(	period_seconds != sampling_scan_parameters.scan_period_seconds || 
		interval_ms != sampling_scan_parameters.scan_interval_ms ||
		window_ms != sampling_scan_parameters.scan_window_ms ||
		duration_seconds != sampling_scan_parameters.scan_duration_seconds || 
		group_filter != sampling_scan_parameters.scan_group_filter || 
		aggregation_type != sampling_scan_parameters.scan_aggregation_type) 
		{
			parameters_changed_sampling = 1;
		}
	
	// Update the parameters
	sampling_scan_parameters.scan_timeout_ms = (!streaming) ? timeout_ms : sampling_scan_parameters.scan_timeout_ms;
	sampling_scan_parameters.scan_stream_timeout_ms = (!streaming) ? timeout_ms : sampling_scan_parameters.scan_stream_timeout_ms;
	sampling_scan_parameters.scan_period_seconds = period_seconds;
	sampling_scan_parameters.scan_interval_ms = interval_ms;	
	sampling_scan_parameters.scan_window_ms = window_ms;	
	sampling_scan_parameters.scan_duration_seconds = duration_seconds;
	sampling_scan_parameters.scan_group_filter = group_filter;
	sampling_scan_parameters.scan_aggregation_type = aggregation_type;
	
	
	
	if(!streaming) {
		if((sampling_configuration & SAMPLING_SCAN) == 0 || parameters_changed_sampling) { 		// Only stop and start the sampling if it is not already running or the parameter changed
			debug_log("SAMPLING: (Re-)start scan sampling\n");
			// Stop the sampling-timer that was probably already started
			app_timer_stop(sampling_scan_timer);
			
			// Now start the sampling-timer
			ret = app_timer_start(sampling_scan_timer, APP_TIMER_TICKS(((uint32_t)period_seconds)*1000, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;	
			
			sampling_configuration = (sampling_configuration_t) (sampling_configuration | SAMPLING_SCAN);
			advertiser_set_status_flag_scan_enabled(1);
			
			timeout_start(scan_timeout_id, timeout_ms);
		} else {
			debug_log("SAMPLING: Ignoring start scan sampling\n");
		}
	} else {
		// If we are already sampling, we need to restart everything, if the parameters changed for sampling
		if((sampling_configuration & SAMPLING_SCAN) && parameters_changed_sampling) {
			debug_log("SAMPLING: (Re-)start scan sampling on stream request (because parameters changed)\n");
			app_timer_stop(sampling_scan_timer);
			ret = app_timer_start(sampling_scan_timer, APP_TIMER_TICKS(((uint32_t)period_seconds)*1000, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;	
			// timeout_start(scan_timeout_id, timeout_ms); // Don't update the timeout for the sampling
		} else if((sampling_configuration & SAMPLING_SCAN) == 0) { // If we are not already sampling the scanner, we have to start the sampling-timer
			debug_log("SAMPLING: Start scan stream\n");
			app_timer_stop(sampling_scan_timer);
			ret = app_timer_start(sampling_scan_timer, APP_TIMER_TICKS(((uint32_t)period_seconds)*1000, 0), NULL);
			if(ret != NRF_SUCCESS) return ret;	
		}  else { // else case: sampling && !parameters_changed
			debug_log("SAMPLING: Nothing to do to start scan stream\n");
		}
			
		sampling_configuration = (sampling_configuration_t) (sampling_configuration | STREAMING_SCAN);
		
		timeout_start(scan_stream_timeout_id, timeout_ms);
	}
	
	
	
	
	return ret;	
}

void sampling_stop_scan(uint8_t streaming) {
	
	// Check if we are allowed to disable the scan timer
	if(!streaming) {
		if((sampling_configuration & STREAMING_SCAN) == 0) {
			app_timer_stop(sampling_scan_timer);
			scanner_stop_scanning();
		}
		sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(SAMPLING_SCAN));
		advertiser_set_status_flag_scan_enabled(0);
	} else {
		if((sampling_configuration & SAMPLING_SCAN) == 0) {
			app_timer_stop(sampling_scan_timer);	
			scanner_stop_scanning();
		}
		sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(STREAMING_SCAN));
	}
	
}

void sampling_scan_callback(void* p_context) {
	if(sampling_configuration & SAMPLING_SCAN) {
		sampling_setup_scan_sampling_chunk();
	}
	// Start the scan procedure:
	ret_code_t ret = scanner_start_scanning(sampling_scan_parameters.scan_interval_ms, sampling_scan_parameters.scan_window_ms, sampling_scan_parameters.scan_duration_seconds);
	// If an error occurs, directly call the timeout-callback
	if(ret != NRF_SUCCESS) sampling_on_scan_timeout_callback();
}

void sampling_on_scan_report_callback(scanner_scan_report_t* scanner_scan_report) {
	debug_log("SAMPLING: Scan report: ID %u, RSSI: %d\n", scanner_scan_report->ID, scanner_scan_report->rssi);

	if(scanner_scan_report->group != sampling_scan_parameters.scan_group_filter) {
		return;
	}

	
	if(sampling_configuration & SAMPLING_SCAN) {
	
		uint8_t prev_seen = 0;
		for(uint32_t i = 0; i < scan_sampling_chunk->scan_result_data_count; i++) {
			if(scan_sampling_chunk->scan_result_data[i].scan_device.ID == scanner_scan_report->ID) { // We already added it
				if(scan_sampling_chunk->scan_result_data[i].count < 255) { // Check if we haven't 255 counts for this device
					
					// Check which aggregation type to use:
					if(sampling_scan_parameters.scan_aggregation_type == SCAN_CHUNK_AGGREGATE_TYPE_MAX) {
						scan_aggregated_rssi[i] = AGGREGATE_SCAN_SAMPLE_MAX(scan_aggregated_rssi[i], scanner_scan_report->rssi);
					} else {	// Use mean
						scan_aggregated_rssi[i] = AGGREGATE_SCAN_SAMPLE_MEAN(scan_aggregated_rssi[i], scanner_scan_report->rssi);
					}				
					scan_sampling_chunk->scan_result_data[i].count++;
				}
				prev_seen = 1;
				break;
			}
		}
		
		if(!prev_seen) {
			if(scan_sampling_chunk->scan_result_data_count < SCAN_SAMPLING_CHUNK_DATA_SIZE) {
				scan_aggregated_rssi[scan_sampling_chunk->scan_result_data_count] = scanner_scan_report->rssi;
				scan_sampling_chunk->scan_result_data[scan_sampling_chunk->scan_result_data_count].scan_device.ID = scanner_scan_report->ID;
				scan_sampling_chunk->scan_result_data[scan_sampling_chunk->scan_result_data_count].scan_device.rssi = 0;	// We could not set it here (it is aggregated)
				scan_sampling_chunk->scan_result_data[scan_sampling_chunk->scan_result_data_count].count = 1;
				scan_sampling_chunk->scan_result_data_count++;	
			}
		}
	}
	
	if(sampling_configuration & STREAMING_SCAN) {
		ScanStream scan_stream;
		scan_stream.scan_device.ID = scanner_scan_report->ID;
		scan_stream.scan_device.rssi = scanner_scan_report->rssi;
		circular_fifo_write(&scan_stream_fifo, (uint8_t*) &scan_stream, sizeof(scan_stream));
	}
	
	
}
*/
