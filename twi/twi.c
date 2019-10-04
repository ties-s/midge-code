#include <stdio.h>
#include "app_util_platform.h"
#include "app_error.h"
#include "nrfx_twim.h"
#include "boards.h"
#include "twi.h"
#include "nrf_log.h"
#include "app_scheduler.h"
#include "nrfx_ppi.h"
#include "nrfx_gpiote.h"
#include "nrfx_timer.h"
#include "ICM20948_driver_interface.h"


#define	ICM_20948_ADD		0x68
#define TWI_INSTANCE_ID		1
static const nrfx_twim_t m_twim = NRFX_TWIM_INSTANCE(TWI_INSTANCE_ID);
static const nrfx_timer_t timer = NRFX_TIMER_INSTANCE(4);

// Semaphore: true if TWI transfer operation has completed
static volatile bool transfer_done = false;
static volatile uint8_t twim_dma_count =0;
static ret_code_t twim_err_code;

uint8_t twi_fifo_buffer[1024] = {};

void timer_event_handler(nrf_timer_event_t event_type, void * p_context)
{
    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
        	NRF_LOG_INFO("timer compare0, :%d", twim_dma_count);

        	break;
        default:
            //Do nothing.
            break;
    }
}

static void twi_event_handler(nrfx_twim_evt_t const * p_event, void * p_context)
{
//	NRF_LOG_INFO("Event type: %x, xfer_desc type: %x", p_event->type, p_event->xfer_desc.type);
//	NRF_LOG_HEXDUMP_INFO(&p_event->type, 4);
	switch (p_event->type)
	{
	case NRFX_TWIM_EVT_DONE:
	{
		NRF_LOG_INFO("%d",p_event->xfer_desc.type);

		switch (p_event->xfer_desc.type)
		{
		case NRFX_TWIM_XFER_TX:
		case NRFX_TWIM_XFER_RX:
		{
			transfer_done = true;
			break;
		}
		case NRFX_TWIM_XFER_TXRX:

			NRF_LOG_INFO("twim handler, :%d", twim_dma_count);
			if (++twim_dma_count < 4)
			{
				nrfx_timer_increment(&timer);
				break;
			}
			app_sched_event_put(NULL, 0, icm20948_service_isr);
			twim_dma_count = 0;
			break;
		default:
			NRF_LOG_INFO("unknown xfer_desc.type: %x\n", p_event->xfer_desc.type);
			break;
		}
		break;
	}
	default:
		NRF_LOG_INFO("Unknown event type: %x\n", p_event->type);
		break;
	}
}


/*
 * void vWaitForEvent(void)
 *
 * For single-threaded systems, this function blocks until the
 * TWI transfer complete interrupt occurs. Whilst waiting, the
 * CPU enters low-power sleep mode.
 *
 * For multi-threaded (RTOS) builds, this function would suspend
 * the thread until the transfer complete event occurs.
 */
static void wait_for_twi_event()
{
	do
	{
		__WFE();
	}
	while (! transfer_done);
}

int twim_write_register(void * context, uint8_t address, const uint8_t * value, uint32_t length)
{
	uint8_t buf[256];
	buf[0] = address;
	memcpy(&buf[1], value, length);

	transfer_done = false;
	twim_err_code = nrfx_twim_tx(&m_twim, ICM_20948_ADD, buf, length+1, false);
	APP_ERROR_CHECK(twim_err_code);

	wait_for_twi_event();

	return (uint8_t)twim_err_code;
}

int twim_read_register(void * context, uint8_t address, uint8_t * buffer, uint32_t length)
{
	(void)context;

	transfer_done = false;
	twim_err_code = nrfx_twim_tx(&m_twim, ICM_20948_ADD, &address, 1, true);
	APP_ERROR_CHECK(twim_err_code);

	wait_for_twi_event();

	transfer_done = false;
	twim_err_code = nrfx_twim_rx(&m_twim, ICM_20948_ADD, buffer, length);
	APP_ERROR_CHECK(twim_err_code);

	wait_for_twi_event();

	return (uint8_t)twim_err_code;
}

ret_code_t twi_init (void)
{
	const nrfx_twim_config_t twim_config = {
			.scl                = TWI_SCL,
			.sda                = TWI_SDA,
			.frequency          = NRF_TWIM_FREQ_400K, //0x0C380D40 - this will set it to 800kHz
			.interrupt_priority = APP_IRQ_PRIORITY_LOW,
			.hold_bus_uninit    = false
	};

	twim_err_code = nrfx_twim_init(&m_twim, &twim_config, twi_event_handler, NULL);
	if (twim_err_code) return twim_err_code;

	nrfx_twim_enable(&m_twim);
//	NRF_LOG_INFO("%x",NRF_TWIM1->ENABLE); // printing 6 verifies DMA usage


	// setup non-blocking fifo read transfer and get task&event
	uint8_t fifo_buffer_address = 0x72;
	nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXRX(ICM_20948_ADD, &fifo_buffer_address, 1, twi_fifo_buffer, 100);
	uint32_t flags = 	NRFX_TWIM_FLAG_HOLD_XFER			|
						NRFX_TWIM_FLAG_RX_POSTINC       	|
						NRFX_TWIM_FLAG_REPEATED_XFER;
	twim_err_code = nrfx_twim_xfer(&m_twim, &xfer, flags);
	if (twim_err_code) return twim_err_code;

    uint32_t twi_task_addr = nrfx_twim_start_task_get(&m_twim, xfer.type);
//    uint32_t twi_stop_evt = nrfx_twim_stopped_event_get(&m_twim);


    // GPIOTE int1 setup & get event
	twim_err_code = nrfx_gpiote_init();
	if (twim_err_code) return twim_err_code;

	nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
	in_config.pull = NRF_GPIO_PIN_PULLUP;

	twim_err_code = nrfx_gpiote_in_init(INT1_PIN, &in_config, NULL);
	if (twim_err_code) return twim_err_code;
	uint32_t int_evt_addr = nrfx_gpiote_in_event_addr_get(INT1_PIN);

	// Initialize LED for output.
    nrfx_gpiote_out_config_t config = NRFX_GPIOTE_CONFIG_OUT_TASK_TOGGLE(1);
    twim_err_code = nrfx_gpiote_out_init(LED, &config);
    APP_ERROR_CHECK(twim_err_code);
    uint32_t gpiote_task_addr = nrfx_gpiote_out_task_addr_get(LED);
    nrfx_gpiote_out_task_enable(LED);


	//Configure timer to count 4 transmissions & get task and event
	nrfx_timer_config_t timer_cfg = NRFX_TIMER_DEFAULT_CONFIG;
	timer_cfg.mode = NRF_TIMER_MODE_COUNTER;
	twim_err_code = nrfx_timer_init(&timer, &timer_cfg, timer_event_handler);
	if (twim_err_code) return twim_err_code;

	// Compare event after every increment
//	nrfx_timer_compare(&timer, NRF_TIMER_CC_CHANNEL0, 1, true);
	// Compare event after 1 transmission
	nrfx_timer_extended_compare(&timer, NRF_TIMER_CC_CHANNEL0, 1, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

	uint32_t timer_count_task = nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_COUNT);
	uint32_t timer_cc_event = nrfx_timer_compare_event_address_get(&timer,  NRF_TIMER_CC_CHANNEL0);
	nrfx_timer_enable(&timer);
	NRF_LOG_INFO("timer configured\n");


	// We need 3 ppi channels: INT1->start twi, TWI_end->Counter increase, Counter=4->TWI stop?
	nrf_ppi_channel_t ppi_channel_int;
	nrf_ppi_channel_t ppi_channel_twim;
//	nrf_ppi_channel_t ppi_channel_timer;

	twim_err_code = nrfx_ppi_channel_alloc(&ppi_channel_int);
	if (twim_err_code) return twim_err_code;
	twim_err_code = nrfx_ppi_channel_alloc(&ppi_channel_twim);
	if (twim_err_code) return twim_err_code;
//	twim_err_code = nrfx_ppi_channel_alloc(&ppi_channel_timer);
//	if (twim_err_code) return twim_err_code;


    twim_err_code = nrfx_ppi_channel_assign(ppi_channel_int, int_evt_addr, timer_count_task);
    if (twim_err_code) return twim_err_code;

    twim_err_code = nrfx_ppi_channel_assign(ppi_channel_twim, timer_cc_event, twi_task_addr);
    if (twim_err_code) return twim_err_code;
    twim_err_code = nrfx_ppi_channel_fork_assign(ppi_channel_twim, gpiote_task_addr); //sanity check that the channel works- it does
    if (twim_err_code) return twim_err_code;



//	twim_err_code = nrfx_ppi_channel_assign(&ppi_channel_timer, timer_cc_event, NRF_TWIM_TASK_STOP);
//	if (twim_err_code) return twim_err_code;

	twim_err_code = nrfx_ppi_channel_enable(ppi_channel_int);
	if (twim_err_code) return twim_err_code;

	twim_err_code = nrfx_ppi_channel_enable(ppi_channel_twim);
	if (twim_err_code) return twim_err_code;

//	twim_err_code = nrfx_ppi_channel_enable(ppi_channel_timer);
//	if (twim_err_code) return twim_err_code;


	return NRF_SUCCESS;
}

