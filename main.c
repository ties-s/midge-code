#include <drv_audio_pdm.h>
#include <request_handler_lib.h>
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
#include "sampling_lib.h"

#include "led/led.h"

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    __disable_irq();
	NRF_LOG_ERROR("error id: %d, pc: %d, info: %d", id, pc, info);
    NRF_LOG_FINAL_FLUSH();

    NVIC_SystemReset();
}


int main(void)
{
	ret_code_t ret;

    NRF_LOG_INIT(NULL);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
	NRF_LOG_INFO("MAIN: Start...\n\r");
	led_init();

	APP_SCHED_INIT(sizeof(data_source_info_t), 50);

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

	led_init_success();

	while(1) {
		app_sched_execute();
		if (NRF_LOG_PROCESS() == false) // no more log entries to process
		{

		}
	}
}


