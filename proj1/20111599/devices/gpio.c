#include "../setting.h"
#include "device.h"

void initDeviceClock(OutputDevice *outdev)
{
	clearGPIOFND(outdev);
	onGPIOLED(outdev->gpio.led_fd, 0xE0);
}
void initDeviceCounter(OutputDevice *outdev)
{
	onGPIOLED(outdev->gpio.led_fd, 0xD0);
}
void turnOffGPIO(OutputDevice *outdev)
{
	clearGPIOFND(outdev);
	offGPIOLED(outdev->gpio.led_fd);
}
void initGPIO(OutputDevice *outdev)	
{
	// GPIO_LED_DEVICE
	outdev->gpio.led_fd = open(GPIO_LED_DEVICE, O_RDWR);
	if(outdev->gpio.led_fd < 0) {
		fprintf(stderr, "Device open error : %s\n", GPIO_LED_DEVICE);
		exit(-1);
	}
	// GPIO_FND_DEVICE
	outdev->gpio.fnd_fd = open(MEM_DEVICE, O_RDWR|O_SYNC);
	if(outdev->gpio.fnd_fd < 0) {
		fprintf(stderr, "Device open error : %s\n", GPIO_FND_DEVICE);
		exit(-1);
	}
	outdev->gpio.gpladdr = (unsigned long *) mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, outdev->gpio.fnd_fd, FND_GPL_ADDR);
	if(outdev->gpio.gpladdr != NULL)
	{
		outdev->gpio.gpl_con = (unsigned long *)(outdev->gpio.gpladdr + FND_GPL_CON);
		outdev->gpio.gpl_dat = (unsigned long *)(outdev->gpio.gpladdr + FND_GPL_DAT);
	}
	if(*(outdev->gpio.gpl_con) ==  (unsigned long)-1|| *(outdev->gpio.gpl_dat) == (unsigned long)-1)
	{
		printf("mmap error!\n");
		close(outdev->gpio.fnd_fd);
		exit(1);
	}

	outdev->gpio.gpe_addr = (unsigned long *)mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_SHARED,outdev->gpio.fnd_fd,FND_GPE_ADDR);
	if(outdev->gpio.gpe_addr != NULL)
	{
		outdev->gpio.gpe_con = (unsigned long *)(outdev->gpio.gpe_addr + FND_GPE_CON);
		outdev->gpio.gpe_dat = (unsigned long *)(outdev->gpio.gpe_addr + FND_GPE_DAT);
	}

	if(*(outdev->gpio.gpe_con) ==  (unsigned long)-1|| *(outdev->gpio.gpe_dat) == (unsigned long)-1)
	{
		printf("mmap error!\n");
		close(outdev->gpio.fnd_fd);
		exit(1);
	}

	*(outdev->gpio.gpl_con) = 0x11111111;
	*(outdev->gpio.gpe_con) = 0x10010110;
}
void clearGPIOFND(OutputDevice *out)
{
	*(out->gpio.gpl_dat) = 0xFF;
	*(out->gpio.gpe_dat) = fnd_loc[5];
}
void writeGPIOFND(OutputDevice *out, int loc, int val)
{
	//if(0 <= val && val < 16)
	*(out->gpio.gpl_dat) = fnd_val[val];
	//else	return;
	//if(0 <= loc && loc < 6)
	*(out->gpio.gpe_dat) = fnd_loc[loc];
	//else 	return;
}
void offGPIOLED(int fd)
{
	int temp = 0xF0;
	write(fd, &temp, sizeof(unsigned char));
}
void onGPIOLED(int fd, unsigned char dat)
{
	write(fd, &dat, sizeof(unsigned char));
}
