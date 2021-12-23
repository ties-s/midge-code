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
#include "nrfx_gpiote.h"
#include "led.h"

static sampling_configuration_t sampling_configuration;

ret_code_t sampling_init(void)
{
	ret_code_t ret = NRF_SUCCESS;
	(void) ret;
	
	sampling_configuration = (sampling_configuration_t) 0;

	// run here since it is needed by both IMU and audio switch
	ret = nrfx_gpiote_init();
	if (ret) return ret;

	// IMU init
	ret = icm20948_init();
	if(ret != NRF_SUCCESS) return ret;

	scanner_init();

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



/************************** IMU ****************************/

ret_code_t sampling_start_imu(uint16_t acc_fsr, uint16_t gyr_fsr, uint16_t datarate)
{
	ret_code_t ret = NRF_SUCCESS;
	
	// accel 2,4,6,8,16 g
	// gyro 250, 500, 1000, 2000 dps
	ret = icm20948_set_fsr((uint32_t)acc_fsr, (uint32_t)gyr_fsr);
	if(ret != NRF_SUCCESS) return ret;

	ret = icm20948_set_datarate(datarate);
	if(ret != NRF_SUCCESS) return ret;	

	ret = storage_open_file(IMU);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = icm20948_enable_sensors();
	if(ret != NRF_SUCCESS) return ret;
	
	sampling_configuration = (sampling_configuration_t) (sampling_configuration | SAMPLING_IMU);
	advertiser_set_status_flag_imu_enabled(1);
	led_update_status();
	
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
	led_update_status();
	
	return ret;
}


/************************** MICROPHONE ****************************/
ret_code_t sampling_start_microphone(nrf_pdm_mode_t mode)
{
	ret_code_t ret = NRF_SUCCESS;
	if (audio_switch_get_position()==OFF)
		return ret;

	ret = drv_audio_init(mode);
	if(ret != NRF_SUCCESS) return ret;

	ret = storage_open_file(AUDIO);
	if(ret != NRF_SUCCESS) return ret;
	
	ret = nrf_drv_pdm_start();
	if(ret != NRF_SUCCESS) return ret;
			
	sampling_configuration = (sampling_configuration_t) (sampling_configuration | SAMPLING_MICROPHONE);
	advertiser_set_status_flag_microphone_enabled(1);
	led_update_status();

	return ret;
}


ret_code_t sampling_stop_microphone(void)
{
	ret_code_t ret = NRF_SUCCESS;

	ret = nrfx_pdm_stop();
	if(ret != NRF_SUCCESS) return ret;
	nrfx_pdm_uninit();

	ret = storage_close_file(AUDIO);
	if(ret != NRF_SUCCESS) return ret;

	sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(SAMPLING_MICROPHONE));
	advertiser_set_status_flag_microphone_enabled(0);
	led_update_status();

	return ret;
}

/************************** SCAN *********************************/

ret_code_t sampling_start_scan(uint16_t interval_ms, uint16_t window_ms)
{
	ret_code_t ret = NRF_SUCCESS;

	ret = storage_open_file(SCANNER);
	if(ret != NRF_SUCCESS) return ret;

	// Start the scan procedure:
	ret = scanner_start_scanning(interval_ms, window_ms);
	if(ret != NRF_SUCCESS) return ret;

	sampling_configuration = (sampling_configuration_t) (sampling_configuration | SAMPLING_SCAN);
	advertiser_set_status_flag_scan_enabled(1);
	led_update_status();
	
	return ret;	
}

ret_code_t sampling_stop_scan(void)
{
	ret_code_t ret = NRF_SUCCESS;

	scanner_stop_scanning();

	ret = storage_close_file(SCANNER);
	if(ret != NRF_SUCCESS) return ret;

	sampling_configuration = (sampling_configuration_t) (sampling_configuration & ~(SAMPLING_SCAN));
	advertiser_set_status_flag_scan_enabled(0);
	led_update_status();

	return ret;
}
