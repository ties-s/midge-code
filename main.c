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

#include "ble_main.h"
#include "saadc.h"
#include "nrf_gpio.h"
#include "boards.h"

#include "drv_audio.h"
#include "fat_test.h"

#include "app_scheduler.h"
#include "app_timer.h"

#include "ICM20948_driver_interface.h"
#include "Icm20948.h"
//#include "twi.h"

static void on_error(void)
{
    NRF_LOG_FINAL_FLUSH();

    // To allow the buffer to be flushed by the host.
    nrf_delay_ms(100);

//    NVIC_SystemReset();
}


void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    NRF_LOG_ERROR("Filename:%s:%d  error code: %d", p_file_name, line_num, error_code);
    on_error();
}


void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    NRF_LOG_ERROR("Received a fault! id: 0x%08x, pc: 0x%08x, info: 0x%08x", id, pc, info);
    on_error();
}


void app_error_handler_bare(uint32_t error_code)
{
    NRF_LOG_ERROR("Received an error: 0x%08x!", error_code);
    on_error();
}



static void log_init(void)
{
    uint32_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for the Power manager.
 */
static void power_management_init(void)
{
    uint32_t err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    app_sched_execute();
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

#define SCHED_MAX_EVENT_DATA_SIZE       APP_TIMER_SCHED_EVENT_DATA_SIZE             /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE                40                                          /**< Maximum number of events in the scheduler queue. More is needed in case of Serialization. */


/**@brief Function for application main entry.
 */
int main(void)
{
    log_init();
    power_management_init();
    saadc_init();
    ble_init();
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
//    if (!sd_init())
//    	drv_audio_init();

//    	NRF_LOG_INFO("event data size: %d", APP_TIMER_SCHED_EVENT_DATA_SIZE);

    icm20948_init();

    NRF_LOG_INFO("\n\nSPCL test APP start\n");

    for (;;)
    {
    	if (int1)
    		inv_icm20948_poll_sensor(&icm_device, (void *)0, print_sensor_data);
        idle_state_handle();
    }
}
