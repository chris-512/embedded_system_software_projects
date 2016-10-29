#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
	int fd[2];
	char buffer[256];
	pipe2(fd, O_NONBLOCK);

	read(fd[0], buffer, sizeof(buffer));

	printf("Non-block IO\n");
	return 0;
}
