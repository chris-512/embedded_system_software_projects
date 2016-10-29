#include <linux/kernel.h>
#include <linux/uaccess.h>

asmlinkage long sys_pack(unsigned char t_intval, unsigned char t_count, long t_option)
{
	long result;
	char fnd_val, fnd_idx;

	// 시간 옵션 = 시작 위치, 시작 값
	int limit = 4;
	while(limit--)
	{
		if(t_option % 10) 
		{
			fnd_val = t_option % 10;
			fnd_idx = limit;
			break;
		}
		t_option /= 10;
	}

	// result 4바이트 구성 = (시작 위치, 시작 값, 시간 간격, 타이머 횟수)
	result = fnd_idx;
	result = result << 24;
	result |= (fnd_val << 16);
	result |= (t_intval << 8);
	result |= t_count;

	return result;
}
