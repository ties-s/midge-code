#ifndef TWI_H_
#define TWI_H_

#define TWI_FIFO_READ_CHUNKS 2
#define TWI_FIFO_READ_LENGTH 255

//extern uint8_t twi_fifo_buffer[1024];

ret_code_t twi_init(void);
int twim_write_register(void * context, uint8_t address, const uint8_t * value, uint32_t length);
int twim_read_register(void * context, uint8_t address, uint8_t * buffer, uint32_t length);
void setup_fifo_burst(void * p_event_data, uint16_t event_size);


#endif //TWI_H_
