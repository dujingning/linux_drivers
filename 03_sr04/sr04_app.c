#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

#define TIMEOUT_MS 500  // 0.5 Second
#define MAX_EVENTS 5
#define CMD_TRIG_EVENT 100

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <device_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    const char* dev_path = argv[1];
    
    int dev_fd = open(dev_path, O_RDWR);
    if (dev_fd < 0) {
        perror("open device");
        exit(EXIT_FAILURE);
    }
    
    struct pollfd fds[MAX_EVENTS];
    memset(fds, 0, sizeof(fds));
    
    fds[0].fd = dev_fd;
    fds[0].events = POLLIN;

    // POLLOUT if need
    
    printf("Monitoring device: %s (Press Ctrl+C to exit)\n", dev_path);
    
    while (1) {

        int ret = poll(fds, 1, TIMEOUT_MS);
        
        if (ret < 0) {
            perror("poll error");
            break;
        } else if (ret == 0) {
            printf("No data available after %f seconds\n", 0.5);

	    // Strategy here
            ioctl(dev_fd, CMD_TRIG_EVENT); // trige event
            continue;
        }
        
	// dev echo event
        if (fds[0].revents & POLLIN) {

            int val = 0;

            ssize_t bytes = read(fds[0].fd, &val, sizeof(val));
            
            printf("Received %d bytes: [%d]\n", bytes, val);
            if (bytes < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                perror("read error");
                break;
            } else if (bytes == 0) {
                printf("Device disconnected\n");
                break;
            }
            printf("get distance: %lf cm\n", val*17/(double)1000000);
	    usleep(10000); // 10ms delay
        }
    }
    close(dev_fd);
    return 0;
}
