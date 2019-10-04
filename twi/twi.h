#ifndef TWI_H_
#define TWI_H_

ret_code_t twi_init(void);
int twim_write_register(void * context, uint8_t address, const uint8_t * value, uint32_t length);
int twim_read_register(void * context, uint8_t address, uint8_t * buffer, uint32_t length);

#endif //TWI_H_
