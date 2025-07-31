#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

// MPU6050 I2C address and registers
#define MPU6050_ADDR        0x68
#define MPU6050_PWR_MGMT_1  0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_GYRO_XOUT_H  0x43
#define MPU6050_TEMP_OUT_H   0x41

// Initialize MPU6050 (wake up)
void mpu6050_init(void);

// Read accelerometer X, Y, Z axis values
void mpu6050_read_accel(int16_t *ax, int16_t *ay, int16_t *az);

// Read gyro X, Y, Z axis values (not used here, but provided)
void mpu6050_read_gyro(int16_t *gx, int16_t *gy, int16_t *gz);

// Read temperature in Celsius
float mpu6050_read_temp(void);

#endif

