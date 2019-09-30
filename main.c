#include <drv_audio_pdm.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_power.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"
#include "boards.h"
#include "app_scheduler.h"

#include "systick_lib.h"
#include "timeout_lib.h"
#include "ble_lib.h"
#include "storage.h"
#include "advertiser_lib.h"
#include "request_handler_lib_02v1.h"
#include "sampling_lib.h"


/**@brief Function that enters a while-true loop if initialization failed.
 *
 * @param[in]	ret				Error code from an initialization function.
 * @param[in]	identifier		Identifier, represents the number of red LED blinks.
 *
 */
void check_init_error(ret_code_t ret, uint8_t identifier) {
	if(ret == NRF_SUCCESS)
		return;
	while(1) {
		for(uint8_t i = 0; i < identifier; i++) {
			nrf_gpio_pin_write(LED, LED_ON);  //turn on LED
			nrf_delay_ms(200);
			nrf_gpio_pin_write(LED, LED_OFF);  //turn off LED
			nrf_delay_ms(200);
		}
		nrf_delay_ms(2000);
	}
}

/**
 * ============================================== MAIN ====================================================
 */
int main(void)
{
	ret_code_t ret;

    NRF_LOG_INIT(NULL);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

	nrf_gpio_cfg_output(LED);
	nrf_gpio_pin_write(LED, LED_OFF);

	NRF_LOG_INFO("MAIN: Start...\n\r");

	nrf_pwr_mgmt_init();

//	SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);
	APP_SCHED_INIT(4, 100);
//	APP_TIMER_INIT(0, 60, NULL); // doing it in systick_init

	ret = systick_init(0);
	check_init_error(ret, 1);

	ret = timeout_init();
	check_init_error(ret, 2);

	ret = ble_init();
	check_init_error(ret, 3);

	ret = storage_init();
	check_init_error(ret, 4);

	ret = sampling_init();
	check_init_error(ret, 5);

	advertiser_init();

	ret = advertiser_start_advertising();
	check_init_error(ret, 6);

	ret = request_handler_init();
	check_init_error(ret, 7);


	// If initialization was successful, blink the green LED 3 times.
	for(uint8_t i = 0; i < 3; i++) {
		nrf_gpio_pin_write(LED, LED_ON);  //turn on LED
		nrf_delay_ms(100);
		nrf_gpio_pin_write(LED, LED_OFF);  //turn off LED
		nrf_delay_ms(100);
	}

	while(1) {
		app_sched_execute();
		if (NRF_LOG_PROCESS() == false) // no more log entries to process
		{
			nrf_pwr_mgmt_run();
		}
	}
}


