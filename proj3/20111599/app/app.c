#include <linux/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main(void)
{
    int fd;

    fd = open("/dev/stopwatch", O_RDWR);

    write(fd, NULL, 0);

    close(fd);
}
