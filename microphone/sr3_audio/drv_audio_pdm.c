#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "nrf_assert.h"
#include "nrf_error.h"
#include "nrf_gpio.h"
#include "nrf_drv_pdm.h"
#include "nrf_log.h"
#include "drv_audio.h"
#include "fat_test.h"
#include "ff.h"
#include "drv_audio.h"
#include "drv_audio_anr.h"
#include "app_scheduler.h"

#include "SEGGER_RTT.h"

pdm_buf_t pdm_buf[PDM_BUF_NUM];
//int16_t sd_buf[PDM_BUF_SIZE] = {};


ret_code_t drv_audio_disable(void)
{
	return nrf_drv_pdm_stop();
}

static void drv_audio_pdm_event_handler(nrfx_pdm_evt_t const * const p_evt)
{
	ret_code_t err_code;
//	NRF_LOG_INFO("%d %d %d", p_evt->error, p_evt->buffer_requested, (p_evt->buffer_released == NULL)? 1:0 );

	if (p_evt->error)
	{
		NRF_LOG_ERROR("pdm handler error %ld", p_evt->error);
		return;
	}

	if(p_evt->buffer_released)
	{
		// NOTE: As soon as the STARTED event is received, the firmware can write the next SAMPLE.PTR value
		// (this register is double-buffered), to ensure continuous operation.

		for (uint8_t l=0; l<PDM_BUF_NUM; l++)
		{
			if (pdm_buf[l].mic_buf == p_evt->buffer_released)
			{
//				NRF_LOG_INFO("buf rel: %d", l);
//				memcpy(sd_buf, p_evt->buffer_released, PDM_BUF_SIZE);
			    err_code = app_sched_event_put(&l, 1, sd_write);
			    APP_ERROR_CHECK(err_code);
				pdm_buf[l].released = true;
				break;
			}
		}
	}

	if(p_evt->buffer_requested)
	{
		for (uint8_t l=0; l<PDM_BUF_NUM; l++)
		{
			if (pdm_buf[l].released)
			{
//				NRF_LOG_INFO("buf req: %d", l);
				err_code = nrfx_pdm_buffer_set(pdm_buf[l].mic_buf, PDM_BUF_SIZE);
				APP_ERROR_CHECK(err_code);
				pdm_buf[l].released = false;
				break;
			}
		}
	}
}

ret_code_t drv_audio_init(void)
{
	for (uint8_t l=0; l<PDM_BUF_NUM; l++)
	{
		pdm_buf[l].released = true;
	}

	nrf_drv_pdm_config_t pdm_cfg = NRF_DRV_PDM_DEFAULT_CONFIG(CONFIG_IO_PDM_CLK, CONFIG_IO_PDM_DATA);

	pdm_cfg.gain_l      = 0x20;//CONFIG_PDM_GAIN;
	pdm_cfg.gain_r      = 0x20;//CONFIG_PDM_GAIN;

	pdm_cfg.mode        = NRF_PDM_MODE_STEREO;
	pdm_cfg.clock_freq	= 0x08400000;

//	pdm_cfg.edge        = NRF_PDM_EDGE_LEFTFALLING;
//	pdm_cfg.edge        = NRF_PDM_EDGE_LEFTRISING;
	 nrf_drv_pdm_init(&pdm_cfg, drv_audio_pdm_event_handler);

	 return nrf_drv_pdm_start();
}
