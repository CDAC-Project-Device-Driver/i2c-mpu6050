#ifndef I2C_H
#define I2C_H

#include <stdint.h>

// Initialize I2C1 peripheral (PB6 = SCL, PB7 = SDA)
void i2c_init(void);

// Write a byte to a device register over I2C
void i2c_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);

// Read multiple bytes from a device register over I2C
void i2c_read_bytes(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len);

#endif

