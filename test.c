#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define DEVICE_FILE "/dev/accelerometre"

int main() {
    int fd;
    char buffer[128];
    int bytes_read;

    fd = open(DEVICE_FILE, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device file");
        return fd;
    }

    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Failed to read from device");
        close(fd);
        _exit(-1);
    }

    buffer[bytes_read] = '\0'; 
    printf("MPU6050 Reading :\n%s", buffer);
    close(fd);
    return EXIT_SUCCESS;
}
