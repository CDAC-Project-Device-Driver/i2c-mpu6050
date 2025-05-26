#include "i2c.h"
#include "stm32f4xx.h"


void i2c_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;     
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;      

   
    GPIOB->MODER &= ~((3 << (6 * 2)) | (3 << (7 * 2)));
    GPIOB->MODER |=  ((2 << (6 * 2)) | (2 << (7 * 2))); 

    GPIOB->OTYPER |= (1 << 6) | (1 << 7);     
    GPIOB->AFR[0] |= (4 << (6 * 4)) | (4 << (7 * 4));  

    I2C1->CR1 &= ~I2C_CR1_PE;  
    I2C1->CR2 = 16;           
    I2C1->CCR = 80;            
    I2C1->TRISE = 17;          
    I2C1->CR1 |= I2C_CR1_PE;   
}


void i2c_start(void) {
    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB));
}


void i2c_stop(void) {
    I2C1->CR1 |= I2C_CR1_STOP;
}


void i2c_write_addr(uint8_t addr) {
    I2C1->DR = addr << 1;              
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;                
    }


void i2c_write_reg(uint8_t reg) {
    while (!(I2C1->SR1 & I2C_SR1_TXE));
    I2C1->DR = reg;
    while (!(I2C1->SR1 & I2C_SR1_TXE));
}


void i2c_write_data(uint8_t data) {
    I2C1->DR = data;
    while (!(I2C1->SR1 & I2C_SR1_BTF));
}


void i2c_read_bytes(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint8_t len) {
    i2c_start();
    i2c_write_addr(dev_addr);
    i2c_write_reg(reg_addr);

    i2c_start();  
    I2C1->DR = (dev_addr << 1) | 1;  
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;

    while (len--) {
        if (len == 0) {
            
            I2C1->CR1 &= ~I2C_CR1_ACK;
            i2c_stop();
        }
        while (!(I2C1->SR1 & I2C_SR1_RXNE));
        *data++ = I2C1->DR;
    }
   
    I2C1->CR1 |= I2C_CR1_ACK;
}

void i2c_write_byte(uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    i2c_start();
    i2c_write_addr(dev_addr);
    i2c_write_reg(reg_addr);
    i2c_write_data(data);
    i2c_stop();
}

