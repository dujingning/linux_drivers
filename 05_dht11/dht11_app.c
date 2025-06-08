
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int fd;

int main(int argc, char **argv) {
  char buf[2];
  int ret;

  int i;

  if (argc != 2) {
    printf("Usage: %s <dev>\n", argv[0]);
    return -1;
  }

  while (1) {
    fd = open(argv[1], O_RDWR | O_NONBLOCK);
    if (fd == -1) {
      printf("can not open file %s\n", argv[1]);
      return -1;
    }
    if (read(fd, buf, 2) == 2)
      printf("get Humidity: %d, Temperature : %d\n", buf[0], buf[1]);
    else
      printf("get dht11: -1\n");

    close(fd);
    sleep(2);
  }

  return 0;
}
