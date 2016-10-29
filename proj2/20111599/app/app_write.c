#include <unistd.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	int fd;
	unsigned char t_intval, t_count;
	long t_option;
	
	// ���� ����
	fd = open("/dev/dev_driver", O_RDWR);
	if(fd < 0) {
		perror("open error\n");
		exit(-1);
	}

	t_intval 	= atoi(argv[1]); // �ð� ����
	t_count		= atoi(argv[2]); // Ÿ�̸� Ƚ��
	t_option	= atoi(argv[3]); // �ð� �ɼ�
	
	// 336�� �ý��� �� ȣ���� ���� sys_pack() ȣ��.
	// �־��� �ð� ����, Ƚ��, �ð� �ɼ��� 4����Ʈ ������� ������.
	long res = syscall(366, t_intval, t_count, t_option);

	// ����� 4����Ʈ ���� ����̽��� ��.
	write(fd, &res, sizeof(res));

	// write �� ��, Ÿ�̸Ӱ� ����� ������ sleep.
	// �̸� �������� ������ close�� �ٷ� ȣ��Ǿ� ���α׷� �̻� ���� �߻� ����.
	usleep((t_count+1) * t_intval * 100000);

	// ���� �ݱ� �� ����̽� �ʱ�ȭ
	close(fd);

	return 0;
}
