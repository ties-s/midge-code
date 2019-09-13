#include <stdio.h>
#include "app_util_platform.h"
#include "app_error.h"
#include "nrfx_twim.h"
#include "boards.h"
#include "twi.h"
#include "nrf_log.h"

#define	ICM_20948_ADD		0x68
#define TWI_INSTANCE_ID		1
static const nrfx_twim_t m_twim = NRFX_TWIM_INSTANCE(TWI_INSTANCE_ID);

// Semaphore: true if TWI transfer operation has completed
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
static void wait_for_twi_event(void)
{
	do
	{
		__WFE();
	}
	while (! transfer_done);
}

int twim_write_register(void * context, uint8_t address, const uint8_t * value, uint32_t length)
{
	uint8_t buf[17];
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
	twim_err_code = nrfx_twim_tx(&m_twim, ICM_20948_ADD, &address, 1, false);
	APP_ERROR_CHECK(twim_err_code);

	wait_for_twi_event();

	transfer_done = false;
//	if (length == 1)
//	{
		twim_err_code = nrfx_twim_rx(&m_twim, ICM_20948_ADD, buffer, length);
		APP_ERROR_CHECK(twim_err_code);
//	}
//	else
//	{
//		nrfx_twim_xfer_desc_t rx_descriptor = NRFX_TWIM_XFER_DESC_RX(ICM_20948_ADD, buffer, length);
//		twim_err_code = nrfx_twim_xfer(&m_twim, &rx_descriptor, NRFX_TWIM_FLAG_RX_POSTINC|NRFX_TWIM_FLAG_REPEATED_XFER);
//		APP_ERROR_CHECK(twim_err_code);
//	}
	wait_for_twi_event();

	return (uint8_t)twim_err_code;
}

void twi_init (void)
{
	const nrfx_twim_config_t twim_config = {
			.scl                = TWI_SCL,
			.sda                = TWI_SDA,
			.frequency          = NRF_TWIM_FREQ_400K,
			.interrupt_priority = APP_IRQ_PRIORITY_LOW,
			.hold_bus_uninit    = false
	};

	twim_err_code = nrfx_twim_init(&m_twim, &twim_config, twi_event_handler, NULL);
	APP_ERROR_CHECK(twim_err_code);

	nrfx_twim_enable(&m_twim);
}

