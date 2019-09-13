/*
  Copyright (c) 2010 - 2017, Nordic Semiconductor ASA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

  2. Redistributions in binary form, except as embedded into a Nordic
     Semiconductor ASA integrated circuit in a product or a software update for
     such product, must reproduce the above copyright notice, this list of
     conditions and the following disclaimer in the documentation and/or other
     materials provided with the distribution.

  3. Neither the name of Nordic Semiconductor ASA nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

  4. This software, with or without modification, must only be used with a
     Nordic Semiconductor ASA integrated circuit.

  5. Any software provided in binary form under this license must not be reverse
     engineered, decompiled, modified and/or disassembled.

  THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "nrf_assert.h"
#include "nrf_error.h"
#include "nrf_gpio.h"
#include "nrf_drv_pdm.h"
//#include "app_debug.h"
#include "nrf_log.h"

#include "drv_audio.h"

#include "fat_test.h"
#include "ff.h"
#include "drv_audio.h"
#include "drv_audio_anr.h"
//#include "drv_audio_dsp.h"
#include "drv_audio_coder.h"
#include "app_scheduler.h"

#if CONFIG_AUDIO_ENABLED

// Sampling rate depends on PDM configuration.
#define SAMPLING_RATE   (1032 * 1000 / 64)

//static uint8_t                      m_skip_buffers;


typedef struct
{
    int16_t  buf[CONFIG_PDM_BUFFER_SIZE_SAMPLES];
    uint16_t samples;
    bool     free;
}pdm_buf_t;

#define PDM_BUF_NUM 6

static pdm_buf_t              m_pdm_buf[PDM_BUF_NUM];

ret_code_t drv_audio_enable(void)
{
	// Skip buffers with invalid data.
//	m_skip_buffers = MAX(1, ROUNDED_DIV((CONFIG_PDM_TRANSIENT_STATE_LEN * SAMPLING_RATE),
//			(1000 * CONFIG_AUDIO_FRAME_SIZE_SAMPLES)));

	return nrf_drv_pdm_start();
}

ret_code_t drv_audio_disable(void)
{
	return nrf_drv_pdm_stop();
}

static void m_audio_process(void * p_event_data, uint16_t event_size)
{
    int16_t         * p_buffer;
    m_audio_frame_t   frame_buf;
    pdm_buf_t       * p_pdm_buf = (pdm_buf_t *)(*(uint32_t *)p_event_data);

    APP_ERROR_CHECK_BOOL(p_event_data != NULL);
    APP_ERROR_CHECK_BOOL(event_size > 0);
    p_buffer = p_pdm_buf->buf;

#if CONFIG_AUDIO_EQUALIZER_ENABLED
    drv_audio_dsp_equalizer((q15_t *)p_buffer, CONFIG_AUDIO_FRAME_SIZE_SAMPLES);
#endif /* CONFIG_AUDIO_EQUALIZER_ENABLED */

#if CONFIG_AUDIO_GAIN_CONTROL_ENABLED
    drv_audio_dsp_gain_control((q15_t *)p_buffer, CONFIG_AUDIO_FRAME_SIZE_SAMPLES);
#endif /* CONFIG_AUDIO_GAIN_CONTROL_ENABLED */

    uint8_t nested;
    app_util_critical_region_enter(&nested);
    drv_audio_coder_encode(p_buffer, &frame_buf);
    p_pdm_buf->free = true;
    app_util_critical_region_exit(nested);


    // Do something with the &frame_buf -> store it

    uint32_t bytes_written;
    FRESULT ff_result = f_write(&audio_file_handle, frame_buf.data, CONFIG_AUDIO_FRAME_SIZE_BYTES, (UINT *) &bytes_written);
    if (ff_result != FR_OK)
    {
        NRF_LOG_INFO("Write failed\r\n.");
    }
    else
    {
        NRF_LOG_INFO("%d bytes written.", bytes_written);
    }


    if (frame_buf.data_size != CONFIG_AUDIO_FRAME_SIZE_BYTES)
    {
        NRF_LOG_WARNING("m_audio_process: size = %d \r\n", frame_buf.data_size);
    }

}

static void drv_audio_pdm_event_handler(nrfx_pdm_evt_t const * const p_evt)
{
//	NRF_LOG_INFO("pdm event handler. req:%d, error:%d", p_evt->buffer_requested, p_evt->error);
	nrfx_err_t error;

//	if (m_skip_buffers)
//	{
//		m_skip_buffers -= 1;
//		return;
//	}

    uint32_t     err_code;
    pdm_buf_t  * p_pdm_buf = NULL;
    uint32_t     pdm_buf_addr;


	ASSERT(p_evt);
	if (p_evt->error == NRFX_PDM_ERROR_OVERFLOW )
	{
		return;
	}
    for(uint32_t i = 0; i < PDM_BUF_NUM; i++)
    {
        if ( m_pdm_buf[i].free == true )
        {

        	if(p_evt->buffer_released)
        	{
//        		m_buffer_handler((int16_t *)p_evt->buffer_released, CONFIG_PDM_BUFFER_SIZE_SAMPLES);
        		m_pdm_buf[i].free    = false;
        		m_pdm_buf[i].samples = CONFIG_PDM_BUFFER_SIZE_SAMPLES;
	            for (uint32_t j = 0; j < CONFIG_PDM_BUFFER_SIZE_SAMPLES; j++)
	            {
	                m_pdm_buf[i].buf[j] = p_evt->buffer_released[j];
	            }

	            p_pdm_buf = &m_pdm_buf[i];
	            pdm_buf_addr = (uint32_t)&m_pdm_buf[i];

	    	    if (p_pdm_buf != NULL)
	    	    {
	    	        err_code = app_sched_event_put(&pdm_buf_addr, sizeof(pdm_buf_t *), m_audio_process);
	    	        APP_ERROR_CHECK(err_code);
	    	    }
	    	    else
	    	    {
	    	        NRF_LOG_WARNING("m_audio_buffer_handler: BUFFER FULL!!\r\n");
	    	    }

	    	    break;
        	}

        	if(p_evt->buffer_requested)
        	{
        		error = nrfx_pdm_buffer_set(m_pdm_buf[i].buf,CONFIG_PDM_BUFFER_SIZE_SAMPLES);
        		ASSERT(error);

        		break;
        	}
        }
    }
}

ret_code_t drv_audio_init(void)
{
    for(uint32_t i = 0; i < PDM_BUF_NUM; i++)
    {
        m_pdm_buf[i].free = true;
    }

	nrf_drv_pdm_config_t pdm_cfg = NRF_DRV_PDM_DEFAULT_CONFIG(CONFIG_IO_PDM_CLK, CONFIG_IO_PDM_DATA);

	pdm_cfg.gain_l      = CONFIG_PDM_GAIN;
	pdm_cfg.gain_r      = CONFIG_PDM_GAIN;

	pdm_cfg.mode        = NRF_PDM_MODE_MONO;

#if   (CONFIG_PDM_MIC == CONFIG_PDM_MIC_LEFT)
	pdm_cfg.edge        = NRF_PDM_EDGE_LEFTFALLING;
#elif (CONFIG_PDM_MIC == CONFIG_PDM_MIC_RIGHT)
	pdm_cfg.edge        = NRF_PDM_EDGE_LEFTRISING;
#else
#error "Value of CONFIG_PDM_MIC is not valid!"
#endif /* (CONFIG_PDM_MIC == CONFIG_PDM_MIC_LEFT) */


	 nrf_drv_pdm_init(&pdm_cfg, drv_audio_pdm_event_handler);


	 return drv_audio_enable();
}
#endif
