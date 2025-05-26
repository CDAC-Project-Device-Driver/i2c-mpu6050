#include "stm32f4xx.h"
#include "uart.h"


void uart2_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN; 

    
    GPIOA->MODER &= ~(3 << (2 * 2));   
    GPIOA->MODER |= (2 << (2 * 2));  
    GPIOA->AFR[0] |= (7 << (4 * 2));   
    
    
    USART2->BRR = 0x0683;

    USART2->CR1 = USART_CR1_TE | USART_CR1_UE; 
}


void uart2_write(char c) {
    while (!(USART2->SR & USART_SR_TXE)); 
    USART2->DR = c;                        
}


void uart2_print(const char *str) {
    while (*str) {
        uart2_write(*str++);
    }
}

