#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
[root@imx6ull:~]# insmod hello_driver.ko
[root@imx6ull:~]# cat /proc/devices|grep hello
245 hello
[root@imx6ull:~]# mknod /dev/hello c 245 0
[root@imx6ull:~]# ./hello_app /dev/hello
read str :
[root@imx6ull:~]# ./hello_app /dev/hello "hello,world!"
write ret = 13
[root@imx6ull:~]# ./hello_app /dev/hello
read str : hello,world!
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
        printf("write ret/len = %d\n", len);
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
