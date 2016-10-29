#include <linux/kernel.h>
#include <linux/uaccess.h>

asmlinkage long sys_pack(unsigned char t_intval, unsigned char t_count, long t_option)
{
	long result;
	char fnd_val, fnd_idx;

	// �ð� �ɼ� = ���� ��ġ, ���� ��
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

	// result 4����Ʈ ���� = (���� ��ġ, ���� ��, �ð� ����, Ÿ�̸� Ƚ��)
	result = fnd_idx;
	result = result << 24;
	result |= (fnd_val << 16);
	result |= (t_intval << 8);
	result |= t_count;

	return result;
}
