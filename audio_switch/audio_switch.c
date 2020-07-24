#include "audio_switch.h"
#include "nrfx_gpiote.h"
#include "spcl.h"
#include "nrf_log.h"
#include "app_scheduler.h"
#include "nrf_gpio.h"
#include "app_timer.h"
#include "sampling_lib.h"


#define DEBOUNCE_TIMEOUT				APP_TIMER_TICKS(3000)
APP_TIMER_DEF(m_debounce_timer_id);

audio_switch_position_t audio_switch_position;

void audio_switch_debounce_timer_handler(void * p_context)
{
	if (nrf_gpio_pin_read(AUDIO_SWITCH_OFF))
	{
		if (sampling_get_sampling_configuration() & SAMPLING_MICROPHONE)
			sampling_stop_microphone();
		audio_switch_position = OFF;
		NRF_LOG_INFO("off debounced");
	}
	else if (nrf_gpio_pin_read(AUDIO_SWITCH_LOW))
	{
		if (sampling_get_sampling_configuration() & SAMPLING_MICROPHONE)
			if (audio_switch_position != LOW)
			{
				sampling_stop_microphone();
				audio_switch_position = LOW;
				sampling_start_microphone(-1);
			}

		audio_switch_position = LOW;
		NRF_LOG_INFO("low debounced");
	}
	else if (nrf_gpio_pin_read(AUDIO_SWITCH_HIGH))
	{
		if (sampling_get_sampling_configuration() & SAMPLING_MICROPHONE)
			if (audio_switch_position != HIGH)
			{
				sampling_stop_microphone();
				audio_switch_position = HIGH;
				sampling_start_microphone(-1);
			}

		audio_switch_position = HIGH;
		NRF_LOG_INFO("high debounced");
	}

	nrfx_gpiote_in_event_enable(AUDIO_SWITCH_OFF, true);
	nrfx_gpiote_in_event_enable(AUDIO_SWITCH_LOW, true);
	nrfx_gpiote_in_event_enable(AUDIO_SWITCH_HIGH, true);
}

void audio_switch_position_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	nrfx_gpiote_in_event_disable(AUDIO_SWITCH_OFF);
	nrfx_gpiote_in_event_disable(AUDIO_SWITCH_LOW);
	nrfx_gpiote_in_event_disable(AUDIO_SWITCH_HIGH);

    app_timer_start(m_debounce_timer_id, DEBOUNCE_TIMEOUT, NULL);
}

audio_switch_position_t audio_switch_get_position(void)
{
	return audio_switch_position;
}

ret_code_t audio_switch_init(void)
{
	ret_code_t err_code;

	nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
	in_config.pull = NRF_GPIO_PIN_PULLDOWN;

	err_code = nrfx_gpiote_in_init(AUDIO_SWITCH_OFF, &in_config, audio_switch_position_handler);
	if (err_code) return err_code;
	err_code = nrfx_gpiote_in_init(AUDIO_SWITCH_LOW, &in_config, audio_switch_position_handler);
	if (err_code) return err_code;
	err_code = nrfx_gpiote_in_init(AUDIO_SWITCH_HIGH, &in_config, audio_switch_position_handler);
	if (err_code) return err_code;

    err_code = app_timer_create(&m_debounce_timer_id, APP_TIMER_MODE_SINGLE_SHOT, audio_switch_debounce_timer_handler);

	nrfx_gpiote_in_event_enable(AUDIO_SWITCH_OFF, true);
	nrfx_gpiote_in_event_enable(AUDIO_SWITCH_LOW, true);
	nrfx_gpiote_in_event_enable(AUDIO_SWITCH_HIGH, true);

	audio_switch_debounce_timer_handler(NULL);

	return NRF_SUCCESS;
}
