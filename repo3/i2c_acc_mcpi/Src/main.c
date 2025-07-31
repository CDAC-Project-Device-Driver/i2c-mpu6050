#include "uart.h"
#include "mpu6050.h"
#include "lcd.h"
#include <stdio.h>
#include <stdint.h>


int16_t ax, ay, az;
float temp;
char buffer[32];

int main(void) {
   
    i2c_init();

    
    uart2_init();

  
    lcd_init();

   
    mpu6050_init();

    while (1) {
       
        mpu6050_read_accel(&ax, &ay, &az);

        
        temp = mpu6050_read_temp();

       
        sprintf(buffer, "AX:%d AY:%d AZ:%d T:%.1fC\n", ax, ay, az, temp);

       
        uart2_print(buffer);

       
        lcd_clear();

   
        sprintf(buffer, "AX:%d AY:%d", ax, ay);
        lcd_print(buffer, 0, 0);  
      
        sprintf(buffer, "AZ:%d T:%.1fC", az, temp);
        lcd_print(buffer, 1, 0);  

      
        for (volatile int i = 0; i < 1000000; i++);
    }
}

