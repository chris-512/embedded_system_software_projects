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
	
	// 파일 개방
	fd = open("/dev/dev_driver", O_RDWR);
	if(fd < 0) {
		perror("open error\n");
		exit(-1);
	}

	t_intval 	= atoi(argv[1]); // 시간 간격
	t_count		= atoi(argv[2]); // 타이머 횟수
	t_option	= atoi(argv[3]); // 시간 옵션
	
	// 336번 시스템 콜 호출을 통해 sys_pack() 호출.
	// 주어진 시간 간격, 횟수, 시간 옵션을 4바이트 결과값을 리턴함.
	long res = syscall(366, t_intval, t_count, t_option);

	// 압축된 4바이트 값을 디바이스에 씀.
	write(fd, &res, sizeof(res));

	// write 한 후, 타이머가 종료될 때까지 sleep.
	// 이를 수행하지 않으면 close가 바로 호출되어 프로그램 이상 현상 발생 가능.
	usleep((t_count+1) * t_intval * 100000);

	// 파일 닫기 및 디바이스 초기화
	close(fd);

	return 0;
}
