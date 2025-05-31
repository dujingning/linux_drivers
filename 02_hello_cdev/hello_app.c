#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


/*
[root@imx6ull:~]# insmod hello_driver.ko
[root@imx6ull:~]# ls /dev/hello
/dev/hello
[root@imx6ull:~]# ls /dev/hello  -l
crw-------    1 root     root      245,   0 Jan  1 00:45 /dev/hello
[root@imx6ull:~]# ./hello_app /dev/hello 123
write ret = 4
[root@imx6ull:~]# ./hello_app /dev/hello
read str : 123
[root@imx6ull:~]# 
*/
int main(int argc, char **argv)
{
    int fd;
    int len;
    char buf[100];

    if (argc < 2)
    {
        printf("Usage: \n");
        printf("%s <dev> [string]\n", argv[0]);
        return -1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0)
    {
        printf("can not open file %s\n", argv[1]);
        return -1;
    }

    if (argc == 3)
    {
        len = write(fd, argv[2], strlen(argv[2])+1);
        printf("write ret = %d\n", len);
    }
    else
    {
        len = read(fd, buf, 100);
        buf[99] = '\0';
        printf("read str : %s\n", buf);
    }

    close(fd);
    return 0;
}
