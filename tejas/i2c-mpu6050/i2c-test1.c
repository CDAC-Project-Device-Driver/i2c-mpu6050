#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>

int main(){
    __uint8_t data[14];
    memset(data,0,14);
    __int16_t prev_ax=0,prev_ay=0,prev_az=0,ax,ay,az,threshold=2000;

    int fd=open("/dev/mp6050",O_RDONLY);
    if(fd<0)
        return -1;

    while(1){
    read(fd,data,6);
    ax = (data[0] << 8) | data[1];
    ay = (data[2] << 8) | data[3];
    az = (data[4] << 8) | data[5];
    
    if(abs(ax-prev_ax)>threshold || abs(ay-prev_ay)>threshold || abs(az-prev_az)>threshold){
        printf("Motion Detected!\n");
    }

    prev_ax=ax;
    prev_ay=ay;
    prev_az=az;
    
    usleep(100000); //100ms
    }
    
    close(fd);
    return 0;
}