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
//static const nrfx_timer_t timer = NRFX_TIMER_INSTANCE(4);

// Semaphore: true if TWI transfer operation has completed
static volatile bool transfer_done = false;
//static volatile uint8_t twim_dma_count =0;
static ret_code_t twim_err_code;

//uint32_t int_evt_addr;
//uint32_t twi_task_addr;
//uint32_t out_task_addr;
//uint32_t twi_stop_evt;
//uint32_t timer_cc_event;
//uint32_t timer_count_task;
//uint32_t ppi_group_disable_task;
//nrf_ppi_channel_t ppi_channel_timer;
//nrf_ppi_channel_t ppi_channel_twim;
//nrf_ppi_channel_t ppi_channel_int;
//nrf_ppi_channel_group_t ppi_channel;

//uint8_t twi_fifo_buffer[TWI_FIFO_READ_CHUNKS*TWI_FIFO_READ_LENGTH];

void timer_event_handler(nrf_timer_event_t event_type, void * p_context)
{
    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
//        	NRF_LOG_INFO("timer compare0, :%d", twim_dma_count);
			app_sched_event_put(NULL, 0, icm20948_service_isr);
        	break;
        default:
            //Do nothing.
            break;
    }
}

static void twi_event_handler(nrfx_twim_evt_t const * p_event, void * p_context)
{
	switch (p_event->type)
	{
	case NRFX_TWIM_EVT_DONE:
	{
		switch (p_event->xfer_desc.type)
		{
		case NRFX_TWIM_XFER_TX:
		case NRFX_TWIM_XFER_RX:
		{
			transfer_done = true;
			break;
		}
//		case NRFX_TWIM_XFER_TXRX:
//
////			NRF_LOG_INFO("twim handler");
////			if (++twim_dma_count < 2)
////			{
////				nrfx_timer_increment(&timer);
////				break;
////			}
//////			nrf_twim_task_trigger(m_twim.p_twim, NRF_TWIM_TASK_STOP);
////			twim_dma_count = 0;
//			app_sched_event_put(NULL, 0, icm20948_service_isr);
////
//			break;
		default:
			NRF_LOG_INFO("unknown xfer_desc.type: %d", p_event->xfer_desc.type);
			break;
		}
		break;
	}
	default:
		NRF_LOG_INFO("Unknown event type: %d", p_event->type);
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

void int_pin_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	// needed the int pin handler, because with just PPI it was too fast WTF- no, ppi int cannot start twim for some fck reason
	//		app_sched_event_put(NULL, 0, setup_fifo_burst);

	if (app_sched_queue_space_get() > 20)
	{
		app_sched_event_put(NULL, 0, icm20948_service_isr);
	}
	else
	{
		NRF_LOG_ERROR("dropped IMU sample");
	}
}

//void setup_fifo_burst(void * p_event_data, uint16_t event_size)
//{
////	nrfx_gpiote_in_event_disable(INT1_PIN);
//
//	uint8_t buf[3];
//	twim_read_register(NULL, 0x70, buf, 2); // fifo count
//	twim_read_register(NULL, 0x1B, &buf[2], 1); // fifo overflow
//	if (buf[2])
//	{
//		buf[0] = 0x1f; buf[1] = 0x1e;
//		twim_write_register(NULL, 0x68, &buf[0], 1);
//		twim_write_register(NULL, 0x68, &buf[1], 1);
//	}
//	NRF_LOG_INFO("fifo size: %d, %d", (uint16_t)(buf[0]<<8|buf[1]), buf[2]);
////	twim_read_register(NULL, 0x72, twi_fifo_buffer, 255);
//
//	// setup non-blocking fifo read transfer and get task&event
//	uint8_t fifo_buffer_address = 0x72;
//	nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXRX(ICM_20948_ADD, &fifo_buffer_address, 1, twi_fifo_buffer, TWI_FIFO_READ_LENGTH);
//	uint32_t flags = 	NRFX_TWIM_FLAG_HOLD_XFER			|
//						NRFX_TWIM_FLAG_RX_POSTINC       	|
//						NRFX_TWIM_FLAG_NO_XFER_EVT_HANDLER	|
//						NRFX_TWIM_FLAG_REPEATED_XFER;
//	twim_err_code = nrfx_twim_xfer(&m_twim, &xfer, flags);
//	if (!twim_err_code)
//	{
//		twi_task_addr = nrfx_twim_start_task_get(&m_twim, xfer.type);
//		twi_stop_evt = nrfx_twim_stopped_event_get(&m_twim);
//	}
//
//	twim_err_code = nrfx_ppi_channel_assign(ppi_channel_twim, twi_stop_evt, twi_task_addr);
//	twim_err_code = nrfx_ppi_channel_fork_assign(ppi_channel_twim, timer_count_task);
//
////	twim_err_code = nrfx_ppi_channel_enable(ppi_channel_twim);
//	twim_err_code = nrfx_ppi_group_enable(ppi_channel);
//
////	if (twim_err_code) return twim_err_code;
////	twim_err_code = nrfx_ppi_channel_assign(ppi_channel_int, int_evt_addr, twi_task_addr);
////	nrfx_ppi_channel_fork_assign(ppi_channel_int, out_task_addr);
////	twim_err_code = nrfx_ppi_channel_enable(ppi_channel_int);
//
//	nrf_twim_task_trigger(m_twim.p_twim, twi_task_addr);
//}

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


	// GPIOTE int1 setup & get event

	nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
	in_config.pull = NRF_GPIO_PIN_PULLUP;

	twim_err_code = nrfx_gpiote_in_init(INT1_PIN, &in_config, int_pin_handler);
//	twim_err_code = nrfx_gpiote_in_init(INT1_PIN, &in_config, NULL);
	if (twim_err_code) return twim_err_code;
//	int_evt_addr = nrfx_gpiote_in_event_addr_get(INT1_PIN);

//	nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_TASK_TOGGLE(1);
//	twim_err_code = nrfx_gpiote_out_init(LED, &out_config);
//	if (twim_err_code) return twim_err_code;
//	out_task_addr = nrfx_gpiote_out_task_addr_get(LED);
//	nrfx_gpiote_out_task_enable(LED);


//	//Configure timer to count 4 transmissions & get task and event
//	nrfx_timer_config_t timer_cfg = NRFX_TIMER_DEFAULT_CONFIG;
//	timer_cfg.mode = NRF_TIMER_MODE_COUNTER;
//	twim_err_code = nrfx_timer_init(&timer, &timer_cfg, timer_event_handler);
//	if (twim_err_code) return twim_err_code;
//	// Compare event after 2 transmissions
//	nrfx_timer_extended_compare(&timer, NRF_TIMER_CC_CHANNEL0, TWI_FIFO_READ_CHUNKS, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
//
//	timer_count_task = nrfx_timer_task_address_get(&timer, NRF_TIMER_TASK_COUNT);
//	timer_cc_event = nrfx_timer_compare_event_address_get(&timer,  NRF_TIMER_CC_CHANNEL0);
//	nrfx_timer_enable(&timer);
//	NRF_LOG_INFO("timer configured\n");
//
//
//	// We need 2 ppi channels: twi_end->next twi, Counter end->stop twi
//	twim_err_code = nrfx_ppi_channel_alloc(&ppi_channel_timer);
//	if (twim_err_code) return twim_err_code;
//	twim_err_code = nrfx_ppi_channel_alloc(&ppi_channel_twim);
//
////	twim_err_code = nrfx_ppi_channel_alloc(&ppi_channel_int);
////	if (twim_err_code) return twim_err_code;
//	twim_err_code = nrfx_ppi_group_alloc(&ppi_channel);
//	if (twim_err_code) return twim_err_code;
//
//	twim_err_code = nrfx_ppi_channel_include_in_group(ppi_channel_twim, ppi_channel);
//	if (twim_err_code) return twim_err_code;
//
//	ppi_group_disable_task = nrfx_ppi_task_addr_group_disable_get(ppi_channel);
//
//	twim_err_code = nrfx_ppi_channel_assign(ppi_channel_timer, timer_cc_event, ppi_group_disable_task);
//	twim_err_code = nrfx_ppi_channel_enable(ppi_channel_timer);

	return NRF_SUCCESS;
}
