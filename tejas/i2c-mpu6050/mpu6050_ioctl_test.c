#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<sys/ioctl.h>
#include"mpu6050_ioctl.h"

int main(int argc,char *argv[]){
    int fd,ret;
    mpu6050_reg_op op={
        .reg=0x75
    };
    
    fd=open("/dev/mpu6050",O_RDWR);
    if(fd<0){
        perror("open() failed");
        _exit(1);
    }

    ret=ioctl(fd,MPU6050_READ_REG,&op);
    if(ret<0){
        perror("ioctl() failed");
        _exit(1);
    }
    printf("Register 0x%02X = 0x%02X\n", op.reg, op.value);

    close(fd);
    return 0;
}