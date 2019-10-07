#ifndef LED_H_
#define LED_H_

void led_init(void);
void check_init_error(ret_code_t ret, uint8_t identifier);
void led_init_success(void);
void led_update_status(void);

#endif /* LED_H_ */
