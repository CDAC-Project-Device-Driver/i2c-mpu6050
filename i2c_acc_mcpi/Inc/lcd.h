#ifndef LCD_H
#define LCD_H

#include <stdint.h>

// Initialize I2C LCD (16x2) with PCF8574 I2C backpack
void lcd_init(void);

// Clear LCD display
void lcd_clear(void);

// Print string on LCD at specified row and column (0-based)
void lcd_print(char *str, uint8_t row, uint8_t col);

#endif

