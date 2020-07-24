#include "nrf_drv_saadc.h"
#include "nrf_drv_ppi.h"
#include "nrf_log.h"
#include "ble_gap.h"
#include "advertiser_lib.h"
#include "app_timer.h"

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS   600                                     /**< Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION    6                                       /**< The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/
#define DIODE_FWD_VOLT_DROP_MILLIVOLTS  270                                     /**< Typical forward voltage drop of the diode . */
#define ADC_RES_10BIT                   1024                                    /**< Maximum digital value for 10-bit ADC conversion. */

#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS) / ADC_RES_10BIT) * ADC_PRE_SCALING_COMPENSATION)

#define BATT_MEAS_LOW_BATT_LIMIT_MV      3000 // Cutoff voltage [mV].
#define BATT_MEAS_FULL_BATT_LIMIT_MV     4200 // Full charge definition [mV].
#define BATT_MEAS_VOLTAGE_TO_SOC_DELTA_MV  11 // mV between each element in the SoC vector.
#define BATT_MEAS_VOLTAGE_TO_SOC_ELEMENTS 111 // Number of elements in the state of charge vector.

/** Converts voltage to state of charge (SoC) [%]. The first element corresponds to the voltage
BATT_MEAS_LOW_BATT_LIMIT_MV and each element is BATT_MEAS_VOLTAGE_TO_SOC_DELTA_MV higher than the previous.
Numbers are obtained via model fed with experimental data. */
static const uint8_t BATT_MEAS_VOLTAGE_TO_SOC[] =
{
 0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,
 2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,
 4,  5,  5,  5,  6,  6,  7,  7,  8,  8,  9,  9, 10, 11, 12, 13, 13, 14, 15, 16,
18, 19, 22, 25, 28, 32, 36, 40, 44, 47, 51, 53, 56, 58, 60, 62, 64, 66, 67, 69,
71, 72, 74, 76, 77, 79, 81, 82, 84, 85, 85, 86, 86, 86, 87, 88, 88, 89, 90, 91,
91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 100
};

static nrf_saadc_value_t 	adc_buf[2];
uint8_t						batt_lvl_in_percentage;

#define BATTERY_LEVEL_MEAS_INTERVAL     APP_TIMER_TICKS(60000)     		            /**< Battery level measurement interval (ticks). */
APP_TIMER_DEF(m_battery_timer_id);                                  				/**< Battery timer. */

static void battery_level_meas_timeout_handler(void * p_context)
{
	uint32_t          	err_code;
	err_code = nrf_drv_saadc_sample();
	APP_ERROR_CHECK(err_code);
}

void saadc_event_handler(nrf_drv_saadc_evt_t const * p_event)
{
	if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
	{
		nrf_saadc_value_t 	adc_result;
		uint32_t          	err_code;
		uint16_t 			batt_lvl_in_milli_volts;

		adc_result = p_event->data.done.p_buffer[0];

		err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 1);
		APP_ERROR_CHECK(err_code);

		batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(adc_result)*2; // *2 due to the voltage divider

		int16_t voltage_vector_element = (batt_lvl_in_milli_volts - BATT_MEAS_LOW_BATT_LIMIT_MV)/BATT_MEAS_VOLTAGE_TO_SOC_DELTA_MV;

		// Ensure that only valid vector entries are used.
	    if (voltage_vector_element < 0)
	    	voltage_vector_element = 0;
	    else if (voltage_vector_element > (BATT_MEAS_VOLTAGE_TO_SOC_ELEMENTS- 1) )
	    	voltage_vector_element = (BATT_MEAS_VOLTAGE_TO_SOC_ELEMENTS- 1);

	    batt_lvl_in_percentage = BATT_MEAS_VOLTAGE_TO_SOC[voltage_vector_element];
	    advertiser_set_battery_percentage(batt_lvl_in_percentage);

//		NRF_LOG_INFO("Battery voltage: %d, percentage: %d%%", batt_lvl_in_milli_volts, batt_lvl_in_percentage);

	}
}


void saadc_init(void)
{
	ret_code_t err_code;
	nrf_saadc_channel_config_t channel_config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN3);
	nrf_drv_saadc_config_t saadc_config = NRF_DRV_SAADC_DEFAULT_CONFIG;
	saadc_config.low_power_mode = true;

	err_code = nrf_drv_saadc_init(&saadc_config, saadc_event_handler);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_channel_init(0, &channel_config);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_buffer_convert(&adc_buf[0], 1);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_saadc_buffer_convert(&adc_buf[1], 1);
	APP_ERROR_CHECK(err_code);

	// request first sample before timer fires.
	err_code = nrf_drv_saadc_sample();
	APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_battery_timer_id, APP_TIMER_MODE_REPEATED, battery_level_meas_timeout_handler);

    err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}
