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

static volatile bool transfer_done = false;
static ret_code_t twim_err_code;


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

int twim_write_register(void * context, uint8_t address, const uint8_t * value, uint32_t length)
{
	uint8_t buf[256];
	buf[0] = address;
	memcpy(&buf[1], value, length);

	transfer_done = false;
	twim_err_code = nrfx_twim_tx(&m_twim, ICM_20948_ADD, buf, length+1, false);
	APP_ERROR_CHECK(twim_err_code);

	while (! transfer_done);

	return (uint8_t)twim_err_code;
}

int twim_read_register(void * context, uint8_t address, uint8_t * buffer, uint32_t length)
{
	(void)context;

	transfer_done = false;
	twim_err_code = nrfx_twim_tx(&m_twim, ICM_20948_ADD, &address, 1, true);
	APP_ERROR_CHECK(twim_err_code);

	while (! transfer_done);

	transfer_done = false;
	twim_err_code = nrfx_twim_rx(&m_twim, ICM_20948_ADD, buffer, length);
	APP_ERROR_CHECK(twim_err_code);

	while (! transfer_done);

	return (uint8_t)twim_err_code;
}

void int_pin_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	// never happens after irqs were sorted out
	if (app_sched_queue_space_get() > 20)
	{
		twim_err_code = app_sched_event_put(NULL, 0, icm20948_service_isr);
		APP_ERROR_CHECK(twim_err_code);
	}
	else
	{
		NRF_LOG_ERROR("dropped IMU sample");
	}
}

ret_code_t twi_init (void)
{
	const nrfx_twim_config_t twim_config = {
			.scl                = TWI_SCL,
			.sda                = TWI_SDA,
			.frequency          = NRF_TWIM_FREQ_400K, //0x0C380D40 - this will set it to 800kHz
			.interrupt_priority = 5,
			.hold_bus_uninit    = false
	};

	twim_err_code = nrfx_twim_init(&m_twim, &twim_config, twi_event_handler, NULL);
	if (twim_err_code) return twim_err_code;

	nrfx_twim_enable(&m_twim);

	// GPIOTE int1 setup & get event

	nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
	in_config.pull = NRF_GPIO_PIN_PULLUP;

	twim_err_code = nrfx_gpiote_in_init(INT1_PIN, &in_config, int_pin_handler);
	if (twim_err_code) return twim_err_code;

	return NRF_SUCCESS;
}
