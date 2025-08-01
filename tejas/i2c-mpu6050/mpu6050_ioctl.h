#ifndef __MPU6050_IOCTL_H
#define ____MPU6050_IOCTL_H

#include<linux/ioctl.h>

typedef struct{
    unsigned char reg;
    unsigned char value;  
}mpu6050_reg_op;

#define MPU6050_WRITE_REG _IOW('x',1,mpu6050_reg_op)
#define MPU6050_READ_REG _IOWR('x',2,mpu6050_reg_op)

#endif