#include <unistd.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// ioctl 명령 제어를 위한 enum 구조체
typedef enum { GPIO_FND_ON = 3, GPIO_LED_ON,
    FPGA_LED_ON, FPGA_DOT_MATRIX_ON,
    FPGA_FND_ON, FPGA_TEXT_LCD_ON} ioctlCmdType;

int main(int argc, char **argv)
{
    int fd;
    unsigned char t_intval, t_count;
    long t_option;

	// 드라이버 개방
    fd = open("/dev/dev_driver", O_RDWR);
    if(fd < 0) {
        perror("open error\n");
        exit(-1);
    }

	// 명령 인자를 받음
    t_intval 	= atoi(argv[1]); // 시간 간격
    t_count		= atoi(argv[2]); // 타이머 횟수
    t_option	= atoi(argv[3]); // 시간 옵션

    int fnd_val, fnd_idx;

	// 시간 옵션으로부터 FND 위치 및 값 추출
    int limit = 4;
    while(limit--)
    {
        if(t_option % 10) {
            fnd_val = t_option % 10;
            fnd_idx = limit;
            break;
        }
        t_option /= 10;
    }

    fnd_val--;

    char student_no[]    = "20111599";
    char student_name[]  = "KIM SAE YOUNG";
	// 학번 및 이름 시작 위치 결정
    long first_row_idx   = 8 - strlen(student_no)		 / 2;
    long second_row_idx  = 8 - strlen(student_name) 	 / 2;
    int dir = -1;
    long data;

    while(t_count--) 
    {
		// 주어진 time quantum 만큼 sleep
        usleep(100000 * t_intval);

        data = (fnd_val << 8) | fnd_idx;
		// ioctl 함수를 통해 GPIO FND를 제어 (주어진 time quantum 단위로 조금씩 수행됨)
        ioctl(fd, GPIO_FND_ON, data);
		// ioctl 함수를 통해 GPIO LED를 제어 
        ioctl(fd, GPIO_LED_ON, data);
		// ioctl 함수를 통해 FPGA FND를 제어 
        ioctl(fd, FPGA_LED_ON, data);
		// ioctl 함수를 통해 FPGA DOT MATRIX를 제어 
        ioctl(fd, FPGA_DOT_MATRIX_ON, data);

        data = t_count;
		// ioctl 함수를 통해 FPGA FND를 제어 
        ioctl(fd, FPGA_FND_ON, data);

        data = (second_row_idx << 8) | first_row_idx;
		// ioctl 함수를 통해 FPGA TEXT LCD를 제어
        ioctl(fd, FPGA_TEXT_LCD_ON, data);

		// 문양 번호 1 증가
        fnd_val = (fnd_val + 1) % 8;
		// 문양 번호가 1로 되돌아오는 순간 문양 위치 1 증가
        if(fnd_val == 0)
            fnd_idx = (fnd_idx + 1) % 4;

		// TEXT LCD 애니메이션을 제어하기 위한 인덱스 조절
        if((dir == 1) && (16 - (second_row_idx + strlen(student_name)) > 0))
        {
            first_row_idx++;
            second_row_idx++;
        }
        else if((dir == -1) && (second_row_idx > 0))
        {
            first_row_idx--;
            second_row_idx--;
        } 
        else {
            dir *= -1;
            if(dir == 1) 
            {
                first_row_idx++;
                second_row_idx++;
            } 
            else 
            {
                first_row_idx--;
                second_row_idx--;
            }   
        }
    }

	// 디바이스를 닫고 초기화
    close(fd);

    return 0;
}
