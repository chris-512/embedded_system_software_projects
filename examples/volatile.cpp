#include <stdio.h>

int func1()
{
	unsigned long *port = (unsigned long *) 0x10000;
	unsigned long value = 0x12;
	*port = value;
	value = *port;
}
int func2()
{
	volatile unsigned long *port = (unsigned long *) 0x10000;
	unsigned long value = 0x12;
	*port = value;
	value = *port;
}
int main(void)
{
}
