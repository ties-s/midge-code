#include "spcl.h"
#include "nrf_error.h"
#include "sampling_lib.h"
#include "nrf_delay.h"

void check_init_error(ret_code_t ret, uint8_t identifier)
{
	if(ret == NRF_SUCCESS)
		return;
	for(uint8_t i = 0; i < identifier; i++)
	{
		nrf_gpio_pin_write(LED, LED_ON);  //turn on LED
		nrf_delay_ms(200);
		nrf_gpio_pin_write(LED, LED_OFF);  //turn off LED
		nrf_delay_ms(200);
	}
	nrf_delay_ms(500);
	// After having blinked the error code, reset. Sometimes the sdcard will recover, and the IMU has another chance of re-calibrating itself
	NVIC_SystemReset();
}

void led_init_success(void)
{
	// If initialization was successful, blink the green LED 3 times.
	for(uint8_t i = 0; i < 3; i++) {
		nrf_gpio_pin_write(LED, LED_ON);  //turn on LED
		nrf_delay_ms(100);
		nrf_gpio_pin_write(LED, LED_OFF);  //turn off LED
		nrf_delay_ms(100);
	}
}

void led_update_status(void)
{
	if (sampling_get_sampling_configuration() != 0)
		nrf_gpio_pin_write(LED, LED_ON);
	else
		nrf_gpio_pin_write(LED, LED_OFF);

}

void led_init(void)
{

	nrf_gpio_cfg_output(LED);
	nrf_gpio_pin_write(LED, LED_OFF);

}

