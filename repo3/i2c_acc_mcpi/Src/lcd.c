#include "lcd.h"
#include "i2c.h"
#include <string.h>

#define LCD_ADDR 0x27   


#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET 0x20
#define LCD_SETDDRAMADDR 0x80

#define LCD_ENTRYLEFT 0x02
#define LCD_DISPLAYON 0x04
#define LCD_2LINE 0x08
#define LCD_5x8DOTS 0x00

static uint8_t backlight = 0x08; 


static void lcd_write4(uint8_t data) {

    i2c_write_byte(LCD_ADDR, 0, data | 0x04 | backlight);
    i2c_write_byte(LCD_ADDR, 0, (data & ~0x04) | backlight);
}

static void lcd_command(uint8_t cmd) {
    uint8_t high_nibble = cmd & 0xF0;
    uint8_t low_nibble = (cmd << 4) & 0xF0;

 
    lcd_write4(high_nibble);
  
    lcd_write4(low_nibble);
}


static void lcd_write_char(uint8_t data) {
    uint8_t high_nibble = data & 0xF0;
    uint8_t low_nibble = (data << 4) & 0xF0;

   
    i2c_write_byte(LCD_ADDR, 0, high_nibble | 0x01 | backlight);
    i2c_write_byte(LCD_ADDR, 0, (high_nibble & ~0x04) | 0x01 | backlight);

    i2c_write_byte(LCD_ADDR, 0, low_nibble | 0x01 | backlight);
    i2c_write_byte(LCD_ADDR, 0, (low_nibble & ~0x04) | 0x01 | backlight);
}

void lcd_init(void) {
  
    for (volatile int i=0; i<100000; i++);

   
    lcd_write4(0x30);
    for (volatile int i=0; i<50000; i++);
    lcd_write4(0x30);
    for (volatile int i=0; i<50000; i++);
    lcd_write4(0x30);
    for (volatile int i=0; i<50000; i++);

    lcd_write4(0x20);  // 4-bit mode
    for (volatile int i=0; i<50000; i++);

    lcd_command(LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS);  
    lcd_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
    lcd_command(LCD_CLEARDISPLAY);
    lcd_command(LCD_ENTRYMODESET | LCD_ENTRYLEFT);

    for (volatile int i=0; i<50000; i++); 
}

void lcd_clear(void) {
    lcd_command(LCD_CLEARDISPLAY);
    for (volatile int i=0; i<20000; i++);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    const uint8_t row_offsets[] = {0x00, 0x40};
    lcd_command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void lcd_print(char *str, uint8_t row, uint8_t col) {
    lcd_set_cursor(row, col);
    while (*str) {
        lcd_write_char(*str++);
    }
}

