#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>

int main(){
    int fd=open("/dev/pchar",O_RDONLY);
    if(fd<0)
        return 0;
    __int8_t data[14];
    memset(data,0,14);
    while(1){
    read(fd,data,14);
     __int16_t accel_x,accel_y,accel_z,temp_raw,gyro_x,gyro_y,gyro_z;
    __int32_t temp_mc,temp_c;
    accel_x = (data[0] << 8) | data[1];
    accel_y = (data[2] << 8) | data[3];
    accel_z = (data[4] << 8) | data[5];
    temp_raw = (data[6] << 8) | data[7];
    gyro_x = (data[8] << 8) | data[9];
    gyro_y = (data[10] << 8) | data[11];
    gyro_z = (data[12] << 8) | data[13];

    printf("accel_x=%d, accel_y=%d, accel_z=%d\n",accel_x/16384,accel_y/16384,accel_z/16384);
    printf("gyro_x=%d, gyro_y=%d, gyro_z=%d\n",gyro_x/131,gyro_y/131,gyro_z/131);
    temp_mc=((__int32_t)temp_raw *1000)/340+36530;
    temp_c=temp_mc/1000;
    printf("temp_raw= %d temp_mc=%d temp_c=%d\n",temp_raw,temp_mc,temp_c);
    printf("Temperature in degrees C = %d.%03d\n",temp_c,temp_mc%1000);
    sleep(5);
    }
    
    close(fd);
    return 0;
}