#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/fs.h>

/**
 * @brief check args
 * 
 * @param step
 * @param delay
 * @return int 0:ok,-1:failed
 */
static int validate_parameters(long step, long delay) {
/*	
    if (step <= 0) {
        fprintf(stderr, "Error: step_number must be positive (got %ld)\n", step);
        return -1;
    }
*/  
    if (delay <= 0) {
        fprintf(stderr, "Error: mdelay_number must be positive (got %ld)\n", delay);
        return -1;
    }
    
    return 0;
}

/**
 * @brief check file type
 * 
 * @param fd
 * @return int 0: char dev, 1: block dev, -1: others dev
 */
static int get_device_type(int fd) {
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat failed");
        return -1;
    }
    
    if (S_ISCHR(st.st_mode)) return 0;     // char dev
    if (S_ISBLK(st.st_mode)) return 1;     // block dev
    
    return -1;  // unknow dev
}

/*
[root@imx6ull:~]# ls
motor_app        motor_driver.ko
[root@imx6ull:~]# insmod motor_driver.ko
[root@imx6ull:~]# ./motor_app /dev/motor 4096 1
Opened character device: /dev/motor
Success: wrote 8 bytes to /dev/motor (step: 4096, delay: 1)
[root@imx6ull:~]#
*/

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <device> <step_number> <mdelay_number>\n", argv[0]);
        fprintf(stderr, "Example: %s /dev/step_motor 200 10\n", argv[0]);
        return EXIT_FAILURE;
    }

    long step = strtol(argv[2], NULL, 0);
    long delay = strtol(argv[3], NULL, 0);
    if (validate_parameters(step, delay) != 0) {
        return EXIT_FAILURE;
    }

    int buf[2] = {(int)step, (int)delay};

    int fd = open(argv[1], O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        fprintf(stderr, "Failed to open %s: %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    int dev_type = get_device_type(fd);
    if (dev_type < 0) {
        close(fd);
        return EXIT_FAILURE;
    }
    
    if (dev_type == 0) {
        printf("Opened character device: %s\n", argv[1]);
    } else {
        printf("Opened block device: %s\n", argv[1]);
        
        unsigned long long size;
        if (ioctl(fd, BLKGETSIZE64, &size) == 0) {
            printf("Device size: %.2f GB\n", size / (1024.0 * 1024 * 1024));
        }
    }

    // write to driver
    ssize_t bytes_written = write(fd, buf, sizeof(buf));
    if (bytes_written < 0) {
        fprintf(stderr, "Write failed: %s\n", strerror(errno));
        close(fd);
        return EXIT_FAILURE;
    }

    if (bytes_written != sizeof(buf)) {
        fprintf(stderr, "Partial write: %zd of %zu bytes written\n", 
                bytes_written, sizeof(buf));
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Success: wrote %zd bytes to %s (step: %d, delay: %d)\n", 
           bytes_written, argv[1], buf[0], buf[1]);
    
    // fsync(fd);  // if nned
    // ioctl(fd, MOTOR_STATUS_CMD, &status); // if need
    
    close(fd);
    return EXIT_SUCCESS;
}
