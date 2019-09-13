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
#include "drv_audio_coder.h"
#include "app_scheduler.h"

#include "SEGGER_RTT.h"


int16_t  mic_buf[PDM_BUFFER] = {};
int16_t  mic_buf2[PDM_BUFFER] = {};

m_audio_frame_t   frame_buf;

ret_code_t drv_audio_enable(void)
{
	return nrf_drv_pdm_start();
}

ret_code_t drv_audio_disable(void)
{
	return nrf_drv_pdm_stop();
}

static void drv_audio_pdm_event_handler(nrfx_pdm_evt_t const * const p_evt)
{
	if (p_evt->error)
	{
		NRF_LOG_INFO("pdm handler error %ld", p_evt->error);
		return;
	}

	if(p_evt->buffer_requested) {

//		uint32_t ret;
		nrfx_pdm_buffer_set(mic_buf, PDM_BUFFER);
//		NRF_LOG_INFO("buffer req: %d",	ret);
	}

	if(p_evt->buffer_released) {


	    uint8_t nested;
	    app_util_critical_region_enter(&nested);
	    drv_audio_coder_encode(p_evt->buffer_released, &frame_buf);
	    app_util_critical_region_exit(nested);

//		NRF_LOG_INFO("buffer release");
//		memcpy(mic_buf2,p_evt->buffer_released,sizeof(mic_buf));

//		buf = p_evt->buffer_released;
	    ret_code_t err_code = app_sched_event_put(NULL, 0, sd_write);
	    APP_ERROR_CHECK(err_code);

//		NRF_LOG_RAW_HEXDUMP_INFO(mic_buf2, 40);
//		NRF_LOG_RAW_HEXDUMP_INFO(buf, PDM_BUFFER*2);
//		NRF_LOG_RAW_INFO("\n");
	}
}

ret_code_t drv_audio_init(void)
{
	nrf_drv_pdm_config_t pdm_cfg = NRF_DRV_PDM_DEFAULT_CONFIG(CONFIG_IO_PDM_CLK, CONFIG_IO_PDM_DATA);

	pdm_cfg.gain_l      = CONFIG_PDM_GAIN;
	pdm_cfg.gain_r      = CONFIG_PDM_GAIN;

	pdm_cfg.mode        = NRF_PDM_MODE_MONO;
//
//#if   (CONFIG_PDM_MIC == CONFIG_PDM_MIC_LEFT)
//	pdm_cfg.edge        = NRF_PDM_EDGE_LEFTFALLING;
//#elif (CONFIG_PDM_MIC == CONFIG_PDM_MIC_RIGHT)
//	pdm_cfg.edge        = NRF_PDM_EDGE_LEFTRISING;
//#else
//#error "Value of CONFIG_PDM_MIC is not valid!"
//#endif /* (CONFIG_PDM_MIC == CONFIG_PDM_MIC_LEFT) */


	 nrf_drv_pdm_init(&pdm_cfg, drv_audio_pdm_event_handler);


	 return drv_audio_enable();
}
