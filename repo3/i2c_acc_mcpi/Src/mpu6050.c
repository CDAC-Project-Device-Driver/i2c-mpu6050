#include "mpu6050.h"
#include "i2c.h"


void mpu6050_init(void) {
    i2c_write_byte(MPU6050_ADDR, MPU6050_PWR_MGMT_1, 0x00);
}


void mpu6050_read_accel(int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t data[6];
    i2c_read_bytes(MPU6050_ADDR, MPU6050_ACCEL_XOUT_H, data, 6);
    *ax = (int16_t)((data[0] << 8) | data[1]);
    *ay = (int16_t)((data[2] << 8) | data[3]);
    *az = (int16_t)((data[4] << 8) | data[5]);
}


void mpu6050_read_gyro(int16_t *gx, int16_t *gy, int16_t *gz) {
    uint8_t data[6];
    i2c_read_bytes(MPU6050_ADDR, MPU6050_GYRO_XOUT_H, data, 6);
    *gx = (int16_t)((data[0] << 8) | data[1]);
    *gy = (int16_t)((data[2] << 8) | data[3]);
    *gz = (int16_t)((data[4] << 8) | data[5]);
}


float mpu6050_read_temp(void) {
    uint8_t data[2];
    i2c_read_bytes(MPU6050_ADDR, MPU6050_TEMP_OUT_H, data, 2);
    int16_t raw_temp = (data[0] << 8) | data[1];
    return (raw_temp / 340.0) + 36.53;
}

