#ifndef MPU6050_IOCTL_H
#define MPU6050_IOCTL_H

#include <linux/ioctl.h>

// Accelerometer axes (X, Y, Z)
#define MPU6050_READ_ACCEL_X    _IOR('x', 1, int16_t)
#define MPU6050_READ_ACCEL_Y    _IOR('x', 2, int16_t)
#define MPU6050_READ_ACCEL_Z    _IOR('x', 3, int16_t)

// Gyroscope axes (X, Y, Z)
#define MPU6050_READ_GYRO_X     _IOR('x', 4, int16_t)
#define MPU6050_READ_GYRO_Y     _IOR('x', 5, int16_t)
#define MPU6050_READ_GYRO_Z     _IOR('x', 6, int16_t)

// Temperature
#define MPU6050_READ_TEMP       _IOR('x', 7, int16_t)

#endif /* MPU6050_IOCTL_H */

