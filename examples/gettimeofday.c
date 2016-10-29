#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	struct timeval start, end;

	time_t tim = time(NULL);
	struct tm *now = localtime(&tim);
	printf("Time is %02d:%02d\n", now->tm_hour, now->tm_min);
}
