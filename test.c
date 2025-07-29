#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include<time.h>
#include<signal.h>

#define READ_INTERVAL_MS 500
#define DEVICE_FILE "/dev/accelerometre"

volatile sig_atomic_t stop = 0;

void handle_sigint(int signum)
{
	stop=1;
}



int main(){
	signal(SIGINT,handle_sigint);
	


 	 int fd;
    	 uint8_t data[14];
   	 fd = open(DEVICE_FILE, O_RDONLY);
    	 if (fd < 0) {
        	perror("Failed to open device file");
        	return fd;
   	 }

 	while(!stop){
    	ssize_t ret = read(fd,data,sizeof(data));
   	if(ret!=sizeof(data))
    	{
    		perror("Failed to read the full sensor data");
		close(fd);
		break;
    	}

   		int16_t accel_x = (data[0] << 8) | data[1];
    		int16_t accel_y = (data[2] << 8) | data[3];
    		int16_t accel_z = (data[4] << 8) | data[5];
    		int16_t temp_raw = (data[6] << 8) | data[7];
   	 	int16_t gyro_x  = (data[8] << 8) | data[9];
    		int16_t gyro_y  = (data[10] << 8) | data[11];
	    	int16_t gyro_z  = (data[12] << 8) | data[13];
	
		float temp_c = (temp_raw/340.0)+36.53;

		float ax = accel_x /16384.0;
		float ay = accel_y /16384.0;
		float az = accel_z /16384.0;

		float gx = gyro_x /131.0;
		float gy = gyro_y /131.0;
		float gz = gyro_z /131.0;

		printf("Accelerometre : X = %.2f g , Y= %.2f g , Z = %.2f g\n",ax,ay,az);
		printf("Gyroscope:     X=%.2f °/s, Y=%.2f °/s, Z=%.2f °/s\n", gx, gy, gz);
   		printf("Temperature:   %.2f °C\n", temp_c);
		usleep(READ_INTERVAL_MS*1000);	
		}

		close(fd);
		printf("\n Stopped Reading Sensor data\n");
		return 0;
	}
	






