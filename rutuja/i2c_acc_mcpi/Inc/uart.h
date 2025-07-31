#ifndef UART_H
#define UART_H

// Initialize UART2 peripheral for TX only, 9600 baud rate
void uart2_init(void);

// Send a single character over UART2
void uart2_write(char c);

// Send a null-terminated string over UART2
void uart2_print(const char *str);

#endif

