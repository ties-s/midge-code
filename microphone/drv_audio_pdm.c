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
#include "audio_switch.h"
#include "sampling_lib.h"

ret_code_t err_code;

nrf_drv_pdm_config_t pdm_cfg = NRF_DRV_PDM_DEFAULT_CONFIG(MIC_CLK, MIC_DOUT);
pdm_buf_t pdm_buf[PDM_BUF_NUM];
data_source_info_t data_source_info;
audio_switch_position_t audio_switch_position;

int16_t subsampled[PDM_BUF_SIZE/DECIMATION];
float filter_weight[DECIMATION];


static void process_audio_buffer(void)
{
	switch (audio_switch_position)
	{
	case OFF:
		break;

	case HIGH:
		if (app_sched_queue_space_get() > 10)
		{
			data_source_info.data_source = AUDIO;
			data_source_info.audio_source_info.audio_buffer_length = PDM_BUF_SIZE*2;
			err_code = app_sched_event_put(&data_source_info, sizeof(data_source_info), sd_write);
			APP_ERROR_CHECK(err_code);
		}
		else
		{
			NRF_LOG_ERROR("dropped audio sample");
			err_code = sampling_stop_microphone();
			err_code |= sampling_start_microphone(-1);
			APP_ERROR_CHECK(err_code);
		}
		break;

	case LOW:
		if (app_sched_queue_space_get() > 10)
		{
			const int16_t * p_sample = data_source_info.audio_source_info.audio_buffer;
			for (uint16_t index=0; index< PDM_BUF_SIZE; index+=(DECIMATION*(pdm_cfg.mode?1:2)))
			{
				float filtered_sample = 0.0, filtered_sample_r = 0.0;
				for (uint8_t inner_index=0; inner_index<(DECIMATION*(pdm_cfg.mode?1:2)); inner_index+=(pdm_cfg.mode?1:2))
				{
					filtered_sample = filtered_sample + filter_weight[inner_index/(pdm_cfg.mode?1:2)]*p_sample[index+inner_index];
					if (!pdm_cfg.mode)
						filtered_sample_r = filtered_sample_r + filter_weight[inner_index/(pdm_cfg.mode?1:2)]*p_sample[index+inner_index+1];
				}
				subsampled[index/DECIMATION] = (int16_t)filtered_sample;
				if (!pdm_cfg.mode)
					subsampled[index/DECIMATION+1] = (int16_t)filtered_sample_r;
			}

			data_source_info.data_source = AUDIO;
			data_source_info.audio_source_info.audio_buffer = subsampled;
			data_source_info.audio_source_info.audio_buffer_length = (PDM_BUF_SIZE/DECIMATION)*2; //sizeof(subsampled);
			err_code = app_sched_event_put(&data_source_info, sizeof(data_source_info), sd_write);
			APP_ERROR_CHECK(err_code);
		}
		else
		{
			NRF_LOG_ERROR("dropped audio sample");
			err_code = sampling_stop_microphone();
			err_code |= sampling_start_microphone(-1);
			APP_ERROR_CHECK(err_code);
		}
		break;
	}
}

static void drv_audio_pdm_event_handler(nrfx_pdm_evt_t const * const p_evt)
{
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
				data_source_info.audio_source_info.audio_buffer = &pdm_buf[l].mic_buf;
				process_audio_buffer();
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
				err_code = nrfx_pdm_buffer_set(pdm_buf[l].mic_buf, PDM_BUF_SIZE);
				APP_ERROR_CHECK(err_code);
				pdm_buf[l].released = false;
				break;
			}
		}
	}
}

ret_code_t drv_audio_init(nrf_pdm_mode_t mode)
{
	static nrf_pdm_mode_t local_mode;
	if (mode != -1) local_mode = mode;

	for (uint8_t l=0; l<PDM_BUF_NUM; l++)
	{
		pdm_buf[l].released = true;
	}

	for (uint8_t f=0; f<DECIMATION; f++)
	{
		filter_weight[f] = 1.0/DECIMATION;
	}

	pdm_cfg.gain_l      = 0x40;
	pdm_cfg.gain_r      = 0x40; // (64/80)= 0x40/0x50

	pdm_cfg.mode        = local_mode;

	// 20kHz
	pdm_cfg.clock_freq	= 0x0A000000;

	nrfx_pdm_init(&pdm_cfg, drv_audio_pdm_event_handler);

	audio_switch_position =	audio_switch_get_position();

    return 0;
}
