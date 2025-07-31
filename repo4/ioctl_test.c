#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include "i2c_mpu6050_ioctl.h"

int main() {
   	
    int fd = open("/dev/accelerometre", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }
    int choice;
    int16_t value;
    while(1){
    	printf("\n MPU_6050 IOCTL SENSOR DATA\n");
    	printf("1. Read Accelerometre X-Axis\n");
    	printf("2. Read Accelerometre Y-Axis\n");
    	printf("3. Read Accelerometre Z-Axis\n");
    	printf("4. Read Temparature \n");
    	printf("5. Read Gyrometre X-Axis\n");
    	printf("6. Read Gyrometre Y-Axis\n");
    	printf("7. Read Gyrometre Z-Axis\n");
    	printf("8. Exit\n");
    	scanf("%d",&choice);
 	int cmd;
        switch (choice) {
            case 1: cmd = MPU6050_READ_ACCEL_X; break;
            case 2: cmd = MPU6050_READ_ACCEL_Y; break;
            case 3: cmd = MPU6050_READ_ACCEL_Z; break;
            case 4: cmd = MPU6050_READ_TEMP;    break;
            case 5: cmd = MPU6050_READ_GYRO_X;  break;
            case 6: cmd = MPU6050_READ_GYRO_Y;  break;
            case 7: cmd = MPU6050_READ_GYRO_Z;  break;
            case 8: close(fd); return 0;
            default:
                printf("Invalid choice!\n");
                continue;
        }

        if (ioctl(fd, cmd, &value) < 0) {
            perror("ioctl failed");
        } else {
            if (cmd == MPU6050_READ_TEMP)
                printf("Temperature = %.2f °C\n", value / 100.0);
            else
                printf("Sensor value = %d\n", value);
        }

        sleep(1);
    }

    close(fd);
    return 0;
}
 

