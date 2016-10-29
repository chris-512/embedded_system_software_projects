#include <unistd.h>
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// ioctl ��� ��� ���� enum ����ü
typedef enum { GPIO_FND_ON = 3, GPIO_LED_ON,
    FPGA_LED_ON, FPGA_DOT_MATRIX_ON,
    FPGA_FND_ON, FPGA_TEXT_LCD_ON} ioctlCmdType;

int main(int argc, char **argv)
{
    int fd;
    unsigned char t_intval, t_count;
    long t_option;

	// ����̹� ����
    fd = open("/dev/dev_driver", O_RDWR);
    if(fd < 0) {
        perror("open error\n");
        exit(-1);
    }

	// ��� ���ڸ� ����
    t_intval 	= atoi(argv[1]); // �ð� ����
    t_count		= atoi(argv[2]); // Ÿ�̸� Ƚ��
    t_option	= atoi(argv[3]); // �ð� �ɼ�

    int fnd_val, fnd_idx;

	// �ð� �ɼ����κ��� FND ��ġ �� �� ����
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
	// �й� �� �̸� ���� ��ġ ����
    long first_row_idx   = 8 - strlen(student_no)		 / 2;
    long second_row_idx  = 8 - strlen(student_name) 	 / 2;
    int dir = -1;
    long data;

    while(t_count--) 
    {
		// �־��� time quantum ��ŭ sleep
        usleep(100000 * t_intval);

        data = (fnd_val << 8) | fnd_idx;
		// ioctl �Լ��� ���� GPIO FND�� ���� (�־��� time quantum ������ ���ݾ� �����)
        ioctl(fd, GPIO_FND_ON, data);
		// ioctl �Լ��� ���� GPIO LED�� ���� 
        ioctl(fd, GPIO_LED_ON, data);
		// ioctl �Լ��� ���� FPGA FND�� ���� 
        ioctl(fd, FPGA_LED_ON, data);
		// ioctl �Լ��� ���� FPGA DOT MATRIX�� ���� 
        ioctl(fd, FPGA_DOT_MATRIX_ON, data);

        data = t_count;
		// ioctl �Լ��� ���� FPGA FND�� ���� 
        ioctl(fd, FPGA_FND_ON, data);

        data = (second_row_idx << 8) | first_row_idx;
		// ioctl �Լ��� ���� FPGA TEXT LCD�� ����
        ioctl(fd, FPGA_TEXT_LCD_ON, data);

		// ���� ��ȣ 1 ����
        fnd_val = (fnd_val + 1) % 8;
		// ���� ��ȣ�� 1�� �ǵ��ƿ��� ���� ���� ��ġ 1 ����
        if(fnd_val == 0)
            fnd_idx = (fnd_idx + 1) % 4;

		// TEXT LCD �ִϸ��̼��� �����ϱ� ���� �ε��� ����
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

	// ����̽��� �ݰ� �ʱ�ȭ
    close(fd);

    return 0;
}
