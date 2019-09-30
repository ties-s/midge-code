#include <drv_audio_pdm.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <storage.h>

#include "nrf_assert.h"
#include "nrf_error.h"
#include "nrf_gpio.h"
#include "nrf_drv_pdm.h"
#include "nrf_log.h"
#include "ff.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "SEGGER_RTT.h"
#include "nrf_delay.h"
#include "boards.h"
#include "storage.h"

ret_code_t err_code;

pdm_buf_t pdm_buf[PDM_BUF_NUM];
static uint16_t skip_samples;
data_source_info_t data_source_info;


static void drv_audio_pdm_event_handler(nrfx_pdm_evt_t const * const p_evt)
{
//	NRF_LOG_INFO("%d %d %d", p_evt->error, p_evt->buffer_requested, (p_evt->buffer_released == NULL)? 1:0 );

	if (p_evt->error)
	{
		NRF_LOG_ERROR("pdm handler error %ld", p_evt->error);
		return;
	}

	if(p_evt->buffer_released)
	{
		for (uint8_t l=0; l<PDM_BUF_NUM; l++)
		{
			if (pdm_buf[l].mic_buf == p_evt->buffer_released)
			{
				pdm_buf[l].released = true;
				if (skip_samples)
				{
					skip_samples--;
				}
				else
				{
					data_source_info.audio_buffer_num = l;
					err_code = app_sched_event_put(&data_source_info, sizeof(data_source_info), sd_write);
					APP_ERROR_CHECK(err_code);
				}
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
	data_source_info.data_source = AUDIO;

	nrf_drv_pdm_config_t pdm_cfg = NRF_DRV_PDM_DEFAULT_CONFIG(MIC_CLK, MIC_DOUT);

	pdm_cfg.gain_l      = 0x40;
	pdm_cfg.gain_r      = 0x40;

	pdm_cfg.mode        = NRF_PDM_MODE_STEREO;
	// 20kHz
	pdm_cfg.clock_freq	= 0x0A000000;

	nrf_drv_pdm_init(&pdm_cfg, drv_audio_pdm_event_handler);

	//should be around 500ms.
	skip_samples = 20000/PDM_BUF_SIZE;
	// divide by 2 for MONO
	if (pdm_cfg.mode) skip_samples = skip_samples/2;

//	nrf_drv_pdm_start();

    return 0;
}
