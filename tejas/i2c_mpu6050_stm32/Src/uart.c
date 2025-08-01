/*
 * uart.c
 *
 *  Created on: Mar 29, 2025
 *      Author: admin
 */

#include "uart.h"

void UartInit(uint32_t baud) {
	// gpio settings
	// enable gpio clock - AHB1ENR
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	// set gpio mode as alt fn - MODER = 10
	GPIOA->MODER |= BV(2*2+1) | BV(2*3+1);
	GPIOA->MODER &= ~(BV(2*2) | BV(2*3));
	// set alt fn - AFRL --> AF7 = UART2
	GPIOA->AFR[0] |= (7 << (4*2)) | (7 << (4*3));
	// disable pull up & pull down - PUPDR
	GPIOA->PUPDR &= ~(BV(2*2) | BV(2*3) | BV(2*2+1) | BV(2*3+1));

	// uart settings
	// enable clock - APB1ENR
	RCC->APB1ENR = RCC_APB1ENR_USART2EN;
	// CR1 = 8N1, Tx En
	USART2->CR1 = USART_CR1_TE;
	// CR2 = 1 stop bit (00) -- default setting
	USART2->CR2 &= ~(USART_CR2_STOP_0 | USART_CR2_STOP_1);
	// set baud rate - BRR
	if(baud == 9600)
		USART2->BRR = 0x0683;
	else if(baud == 38400)
		USART2->BRR = 0x01A1;
	else if(baud == 115200)
		USART2->BRR = 0x008B;
	// enable uart - CR1
	USART2->CR1 |= USART_CR1_UE;
	// enable uart intr in NVIC - ISER regr
	NVIC_EnableIRQ(USART2_IRQn);
}

static int tx_index;
static char *tx_str;
volatile static int tx_completed = 1;

void UartPuts(char *str) {
	// wait until prev string is transmitted
	while(tx_completed == 0)
		;
	// write first char into TDR
	tx_str = str;
	tx_index = 0;
	tx_completed = 0; // start new str tx
	USART2->DR = tx_str[tx_index];
	// enable tx intr - CR1
	USART2->CR1 |= USART_CR1_TXEIE;
}

void USART2_IRQHandler(void) {
	// confirm if tx intr is raised -- look at TXE flag in SR
	if((USART2->SR & USART_SR_TXE) != 0) {
		// go to next char
		tx_index++;
		// if \0 char, disable tx intr - CR1
		if(tx_str[tx_index] == '\0') {
			USART2->CR1 &= ~USART_CR1_TXEIE;
			tx_completed = 1;
		// else send next char to TDR
		} else
			USART2->DR = tx_str[tx_index];
	}
	// check if rx intr is raised -- (when rx intr is enabled and new char is received)
	//if((USART2->SR & USART_SR_RXNE) != 0) {
	//	// write rx intr handling logic here
	//}
}

